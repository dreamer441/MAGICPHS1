#include "SpellExecutionSubsystem.h"

#include "EarthSpellSpawner.h"
#include "LiveSpellSessionSubsystem.h"
#include "PlayerViewModeSubsystem.h"
#include "SpellCastPlacement.h"
#include "SpellLoadoutSubsystem.h"
#include "SpellPatternResolver.h"
#include "SpellMotionResolver.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"

TStatId USpellExecutionSubsystem::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(USpellExecutionSubsystem, STATGROUP_Tickables);
}

void USpellExecutionSubsystem::SetExecutionSuspended(const bool bInSuspended)
{
    if (bInSuspended)
    {
        ++ExecutionSuspensionDepth;
    }
    else if (ExecutionSuspensionDepth > 0)
    {
        --ExecutionSuspensionDepth;
    }
}

void USpellExecutionSubsystem::Tick(const float DeltaSeconds)
{
    if (IsExecutionSuspended())
    {
        return;
    }

    UWorld* World = GetWorld();

    if (!World || !World->IsGameWorld())
    {
        return;
    }

    APlayerController* Controller =
        World->GetFirstPlayerController();

    if (!Controller)
    {
        return;
    }

    static const FKey SlotKeys[USpellLoadoutSubsystem::SlotCount] =
    {
        EKeys::One, EKeys::Two, EKeys::Three, EKeys::Four, EKeys::Five,
        EKeys::Six, EKeys::Seven, EKeys::Eight, EKeys::Nine, EKeys::Zero
    };

    if (USpellLoadoutSubsystem* Loadout =
        World->GetSubsystem<USpellLoadoutSubsystem>())
    {
        if (ULiveSpellSessionSubsystem* LiveSession =
            World->GetSubsystem<ULiveSpellSessionSubsystem>())
        {
            for (int32 SlotIndex = 0;
                 SlotIndex < USpellLoadoutSubsystem::SlotCount;
                 ++SlotIndex)
            {
                if (Controller->WasInputKeyJustPressed(
                    SlotKeys[SlotIndex]))
                {
                    FSpellDefinition PreparedSpell;

                    if (Loadout->GetSlotSpell(
                        SlotIndex,
                        PreparedSpell))
                    {
                        Loadout->EquipSlot(SlotIndex);
                        LiveSession->LoadPreparedSpell(
                            PreparedSpell);
                    }

                    break;
                }
            }
        }
    }

    bool bCastPressed =
        Controller->WasInputKeyJustPressed(
            EKeys::SpaceBar);

    if (const UPlayerViewModeSubsystem* Views =
        World->GetSubsystem<UPlayerViewModeSubsystem>())
    {
        switch (Views->GetCurrentMode())
        {
            case EPlayerGameplayMode::FirstPerson:
                // State 1 owns Space as physical Jump.
                // Hands own LMB/RMB, so no spell execution input lives here yet.
                bCastPressed = false;
                break;

            case EPlayerGameplayMode::ThirdPerson:
                // State 2 owns Space as Jump and LMB as precision spell cast.
                bCastPressed =
                    Controller->WasInputKeyJustPressed(
                        EKeys::LeftMouseButton);
                break;

            case EPlayerGameplayMode::TopDown:
            case EPlayerGameplayMode::FreeRoam:
            default:
                // States 3/4 retain Space execution.
                break;
        }
    }

    if (bCastPressed)
    {
        ExecuteLiveSpell();
    }
}

FSpellExecutionResult
USpellExecutionSubsystem::ExecuteLiveSpell()
{
    if (IsExecutionSuspended())
    {
        return FSpellExecutionResult::ForOutcome(
            ESpellExecutionOutcome::ExecutionSuspended);
    }

    UWorld* World = GetWorld();

    if (!World)
    {
        return FSpellExecutionResult::ForOutcome(
            ESpellExecutionOutcome::NoWorld);
    }

    APlayerController* Controller =
        World->GetFirstPlayerController();

    if (!Controller)
    {
        return FSpellExecutionResult::ForOutcome(
            ESpellExecutionOutcome::NoController);
    }

    ULiveSpellSessionSubsystem* LiveSession =
        World->GetSubsystem<ULiveSpellSessionSubsystem>();

    if (!LiveSession || !LiveSession->CanCast())
    {
        return FSpellExecutionResult::ForOutcome(
            ESpellExecutionOutcome::NoLiveConstruction);
    }

    const FResolvedSpell ResolvedSpell =
        LiveSession->ResolveGenericSpell();

    FVector AimDirection =
        FVector::ForwardVector;

    FVector ViewLocation;
    FRotator ViewRotation;

    Controller->GetPlayerViewPoint(
        ViewLocation,
        ViewRotation);

    // In third person this is exactly the center reticle direction because
    // the active view target is the dedicated shoulder camera.
    AimDirection =
        ViewRotation.Vector().GetSafeNormal();

    if (const UPlayerViewModeSubsystem* Views =
        World->GetSubsystem<UPlayerViewModeSubsystem>())
    {
        FVector ModeOrigin;
        FVector ModeDirection;

        if (Views->GetGameplayCastRay(
            ModeOrigin,
            ModeDirection))
        {
            AimDirection =
                ModeDirection.GetSafeNormal();
        }
    }

    FResolvedSpellCastPlacement Placement;

    if (!FSpellCastPlacement::Resolve(
        Controller,
        ResolvedSpell,
        AimDirection,
        Placement))
    {
        return FSpellExecutionResult::ForOutcome(
            ESpellExecutionOutcome::PlacementFailed);
    }

    TArray<FResolvedPatternInstance> ResolvedInstances;

    FSpellPatternResolver::ResolveInstances(
        ResolvedSpell.Definition.Pattern,
        Placement.SpawnLocation,
        Placement.LaunchDirection,
        ResolvedInstances);

    const FQuat BaseSpellRotation =
        Placement.SpawnRotation.Quaternion();

    int32 SpawnedCount = 0;

    switch (ResolvedSpell.Definition.Element)
    {
        case ESpellElement::Earth:
            for (const FResolvedPatternInstance& Instance :
                 ResolvedInstances)
            {
                const FQuat FinalRotation =
                    Instance.PatternRotation *
                    BaseSpellRotation;

                const FVector InstanceLaunchDirection =
                    FSpellMotionResolver::ResolveDirection(
                        ResolvedSpell.Definition.MotionDirection,
                        ResolvedSpell.Definition.Pattern,
                        Instance.Location,
                        Placement.SpawnLocation,
                        Placement.LaunchDirection);

                if (FEarthSpellSpawner::SpawnAndLaunch(
                    World,
                    Controller,
                    ResolvedSpell,
                    Instance.Location,
                    FinalRotation.Rotator(),
                    InstanceLaunchDirection))
                {
                    ++SpawnedCount;
                }
            }
            break;

        default:
            return FSpellExecutionResult::ForOutcome(
                ESpellExecutionOutcome::UnsupportedElement);
    }

    if (SpawnedCount <= 0)
    {
        return FSpellExecutionResult::ForOutcome(
            ESpellExecutionOutcome::SpawnFailed);
    }

    LiveSession->ResetSession();

    return FSpellExecutionResult::ForOutcome(
        ESpellExecutionOutcome::Executed,
        true);
}
