#include "PlayerViewModeSubsystem.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "InnerRealmSubsystem.h"
#include "InputCoreTypes.h"
#include "Modes/FirstPerson/FirstPersonModeNode.h"
#include "Modes/FreeRoam/FreeRoamModeNode.h"
#include "Modes/ThirdPerson/ThirdPersonModeNode.h"
#include "Modes/TopDown/TopDownModeNode.h"
#include "SpellCastingBindingSubsystem.h"

TStatId UPlayerViewModeSubsystem::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UPlayerViewModeSubsystem, STATGROUP_Tickables);
}

void UPlayerViewModeSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    FirstPersonNode = NewObject<UFirstPersonModeNode>(this);
    ThirdPersonNode = NewObject<UThirdPersonModeNode>(this);
    TopDownNode = NewObject<UTopDownModeNode>(this);
    FreeRoamNode = NewObject<UFreeRoamModeNode>(this);

    CurrentMode = EPlayerGameplayMode::ThirdPerson;
    bModeInitialized = false;
    bWasMeditating = false;
}

void UPlayerViewModeSubsystem::Deinitialize()
{
    UWorld* World = GetWorld();
    APlayerController* PC =
        World ? World->GetFirstPlayerController() : nullptr;

    if (bModeInitialized)
    {
        if (UPlayerGameplayModeNode* Active = GetActiveNode())
        {
            Active->Exit(PC);
        }
    }

    if (World)
    {
        if (USpellCastingBindingSubsystem* Bindings =
            World->GetSubsystem<USpellCastingBindingSubsystem>())
        {
            Bindings->SetTopDownLiveInputEnabled(false);
        }
    }

    FirstPersonNode = nullptr;
    ThirdPersonNode = nullptr;
    TopDownNode = nullptr;
    FreeRoamNode = nullptr;

    bModeInitialized = false;
    Super::Deinitialize();
}

UPlayerGameplayModeNode* UPlayerViewModeSubsystem::GetNode(
    const EPlayerGameplayMode Mode) const
{
    switch (Mode)
    {
        case EPlayerGameplayMode::FirstPerson:
            return FirstPersonNode;

        case EPlayerGameplayMode::ThirdPerson:
            return ThirdPersonNode;

        case EPlayerGameplayMode::TopDown:
            return TopDownNode;

        case EPlayerGameplayMode::FreeRoam:
            return FreeRoamNode;

        default:
            return nullptr;
    }
}

UPlayerGameplayModeNode* UPlayerViewModeSubsystem::GetActiveNode() const
{
    return GetNode(CurrentMode);
}

FPlayerModeCapabilities
UPlayerViewModeSubsystem::GetCurrentCapabilities() const
{
    if (const UPlayerGameplayModeNode* Active = GetActiveNode())
    {
        return Active->GetCapabilities();
    }

    return FPlayerModeCapabilities();
}

void UPlayerViewModeSubsystem::ApplyCapabilities()
{
    UWorld* World = GetWorld();

    if (!World)
    {
        return;
    }

    const FPlayerModeCapabilities Caps =
        GetCurrentCapabilities();

    if (USpellCastingBindingSubsystem* Bindings =
        World->GetSubsystem<USpellCastingBindingSubsystem>())
    {
        Bindings->SetTopDownLiveInputEnabled(
            Caps.bLiveCasting);
    }
}

void UPlayerViewModeSubsystem::AnnounceMode() const
{
    if (!GEngine)
    {
        return;
    }

    FString Message;

    switch (CurrentMode)
    {
        case EPlayerGameplayMode::FirstPerson:
            Message = TEXT(
                "FIRST PERSON | WASD | Space jump | tap Shift repeatedly to sprint | Up: Third Person");
            break;

        case EPlayerGameplayMode::ThirdPerson:
            Message = TEXT(
                "THIRD PERSON | WASD | Space jump | LMB cast | hold RMB shoulder aim | Shift pulse sprint");
            break;

        case EPlayerGameplayMode::TopDown:
            Message = TEXT(
                "TOP DOWN | existing casting preserved | RMB move | LMB orbit | Down: Third | Up: Free Roam");
            break;

        case EPlayerGameplayMode::FreeRoam:
            Message = TEXT(
                "FREE ROAM | WASD + Q/E fly | LMB look | RMB tap body move / hold 3s teleport | Space cast");
            break;

        default:
            break;
    }

    GEngine->AddOnScreenDebugMessage(
        -1,
        3.0f,
        FColor::Cyan,
        Message);
}

void UPlayerViewModeSubsystem::TransitionToMode(
    const EPlayerGameplayMode NewMode,
    APlayerController* PC)
{
    if (!PC)
    {
        return;
    }

    if (bModeInitialized && NewMode == CurrentMode)
    {
        return;
    }

    if (bModeInitialized)
    {
        if (UPlayerGameplayModeNode* Active =
            GetActiveNode())
        {
            Active->Exit(PC);
        }
    }

    CurrentMode = NewMode;

    if (UPlayerGameplayModeNode* Next =
        GetActiveNode())
    {
        Next->Enter(PC);
        bModeInitialized = true;
        ApplyCapabilities();
        AnnounceMode();
    }
}

void UPlayerViewModeSubsystem::StepMode(
    const int32 Direction,
    APlayerController* PC)
{
    const int32 CurrentIndex =
        static_cast<int32>(CurrentMode);

    const int32 TargetIndex =
        FMath::Clamp(
            CurrentIndex + Direction,
            0,
            3);

    if (TargetIndex == CurrentIndex)
    {
        return;
    }

    TransitionToMode(
        static_cast<EPlayerGameplayMode>(
            TargetIndex),
        PC);
}

bool UPlayerViewModeSubsystem::GetGameplayCastRay(
    FVector& OutOrigin,
    FVector& OutDirection) const
{
    if (!bModeInitialized)
    {
        return false;
    }

    const UPlayerGameplayModeNode* Active =
        GetActiveNode();

    return Active &&
        Active->GetCastRay(
            OutOrigin,
            OutDirection);
}

bool UPlayerViewModeSubsystem::GetTopDownCastRay(
    FVector& OutOrigin,
    FVector& OutDirection) const
{
    if (!bModeInitialized ||
        CurrentMode != EPlayerGameplayMode::TopDown)
    {
        return false;
    }

    return GetGameplayCastRay(
        OutOrigin,
        OutDirection);
}

bool UPlayerViewModeSubsystem::GetFreeRoamInteractionPoint(
    FVector& OutPoint) const
{
    if (!bModeInitialized ||
        CurrentMode != EPlayerGameplayMode::FreeRoam)
    {
        return false;
    }

    const UPlayerGameplayModeNode* Active =
        GetActiveNode();

    return Active &&
        Active->GetInteractionPoint(
            OutPoint);
}

void UPlayerViewModeSubsystem::Tick(
    const float DeltaSeconds)
{
    UWorld* World = GetWorld();

    if (!World || !World->IsGameWorld())
    {
        return;
    }

    APlayerController* PC =
        World->GetFirstPlayerController();

    if (!PC)
    {
        return;
    }

    if (!bModeInitialized)
    {
        TransitionToMode(
            CurrentMode,
            PC);
    }

    const UInnerRealmSubsystem* Realm =
        World->GetSubsystem<UInnerRealmSubsystem>();

    const bool bMeditating =
        Realm && Realm->IsActive();

    if (bMeditating)
    {
        bWasMeditating = true;
        return;
    }

    if (bWasMeditating)
    {
        bWasMeditating = false;

        if (UPlayerGameplayModeNode* Active =
            GetActiveNode())
        {
            Active->ResumeAfterExternalView(
                PC);
        }
    }

    if (PC->WasInputKeyJustPressed(EKeys::Up))
    {
        StepMode(+1, PC);
        return;
    }

    if (PC->WasInputKeyJustPressed(EKeys::Down))
    {
        StepMode(-1, PC);
        return;
    }

    if (UPlayerGameplayModeNode* Active =
        GetActiveNode())
    {
        Active->TickMode(
            PC,
            DeltaSeconds);
    }
}
