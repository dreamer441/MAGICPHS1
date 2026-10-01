#include "Phase1ViewModeSubsystem.h"

#include "Core/Phase1ModeNode.h"
#include "Modes/FirstPersonModeNode.h"
#include "Modes/FreeRoamModeNode.h"
#include "Modes/ThirdPersonModeNode.h"
#include "Modes/TopDownModeNode.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"

void UPhase1ViewModeSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    FirstPersonNode = new FFirstPersonModeNode();
    ThirdPersonNode = new FThirdPersonModeNode();
    TopDownNode = new FTopDownModeNode();
    FreeRoamNode = new FFreeRoamModeNode();

    CurrentMode = EPhase1GameplayMode::ThirdPerson;
    bModeInitialized = false;
}

void UPhase1ViewModeSubsystem::Deinitialize()
{
    if (UWorld* World = GetWorld())
    {
        if (APlayerController* PC = World->GetFirstPlayerController())
        {
            if (bModeInitialized)
            {
                if (FPhase1ModeNode* Active = GetActiveNode())
                {
                    Active->Exit(PC);
                }
            }
        }
    }

    delete FirstPersonNode;
    delete ThirdPersonNode;
    delete TopDownNode;
    delete FreeRoamNode;

    FirstPersonNode = nullptr;
    ThirdPersonNode = nullptr;
    TopDownNode = nullptr;
    FreeRoamNode = nullptr;
    bModeInitialized = false;

    Super::Deinitialize();
}

TStatId UPhase1ViewModeSubsystem::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UPhase1ViewModeSubsystem, STATGROUP_Tickables);
}

FPhase1ModeNode* UPhase1ViewModeSubsystem::GetNode(const EPhase1GameplayMode Mode) const
{
    switch (Mode)
    {
        case EPhase1GameplayMode::FirstPerson: return FirstPersonNode;
        case EPhase1GameplayMode::ThirdPerson: return ThirdPersonNode;
        case EPhase1GameplayMode::TopDown: return TopDownNode;
        case EPhase1GameplayMode::FreeRoam: return FreeRoamNode;
        default: return nullptr;
    }
}

FPhase1ModeNode* UPhase1ViewModeSubsystem::GetActiveNode() const
{
    return GetNode(CurrentMode);
}

FPhase1ModeCapabilities UPhase1ViewModeSubsystem::GetCurrentCapabilities() const
{
    if (const FPhase1ModeNode* Active = GetActiveNode())
    {
        return Active->GetCapabilities();
    }

    return FPhase1ModeCapabilities();
}

bool UPhase1ViewModeSubsystem::GetGameplayCastRay(
    FVector& OutOrigin,
    FVector& OutDirection) const
{
    if (!bModeInitialized)
    {
        return false;
    }

    UWorld* World = GetWorld();
    APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
    FPhase1ModeNode* Active = GetActiveNode();

    return PC && Active && Active->GetCastRay(PC, OutOrigin, OutDirection);
}

bool UPhase1ViewModeSubsystem::GetFreeRoamInteractionPoint(FVector& OutPoint) const
{
    if (!bModeInitialized || CurrentMode != EPhase1GameplayMode::FreeRoam)
    {
        return false;
    }

    const FPhase1ModeNode* Active = GetActiveNode();
    return Active && Active->GetInteractionPoint(OutPoint);
}

void UPhase1ViewModeSubsystem::TransitionToMode(
    const EPhase1GameplayMode NewMode,
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
        if (FPhase1ModeNode* Active = GetActiveNode())
        {
            Active->Exit(PC);
        }
    }

    CurrentMode = NewMode;

    if (FPhase1ModeNode* Next = GetActiveNode())
    {
        Next->Enter(PC);
        bModeInitialized = true;
        AnnounceMode();
    }
}

void UPhase1ViewModeSubsystem::StepMode(
    const int32 Direction,
    APlayerController* PC)
{
    const int32 CurrentIndex = static_cast<int32>(CurrentMode);
    const int32 TargetIndex = FMath::Clamp(CurrentIndex + Direction, 0, 3);

    if (TargetIndex == CurrentIndex)
    {
        return;
    }

    TransitionToMode(static_cast<EPhase1GameplayMode>(TargetIndex), PC);
}

void UPhase1ViewModeSubsystem::AnnounceMode() const
{
    if (!GEngine)
    {
        return;
    }

    FString Message;

    switch (CurrentMode)
    {
        case EPhase1GameplayMode::FirstPerson:
            Message = TEXT("STATE 1 / FIRST PERSON | WASD | mouse look | Space jump | Shift pulse sprint | Up: State 2");
            break;

        case EPhase1GameplayMode::ThirdPerson:
            Message = TEXT("STATE 2 / THIRD PERSON | WASD | mouse look | Space jump | RMB shoulder aim | LMB reserved for cast");
            break;

        case EPhase1GameplayMode::TopDown:
            Message = TEXT("STATE 3 / TOP DOWN | RMB move | LMB orbit | wheel-up sprint pulse | world-cursor casting contract");
            break;

        case EPhase1GameplayMode::FreeRoam:
            Message = TEXT("STATE 4 / FREE ROAM | WASD + Q/E camera | LMB look | RMB tap body move / hold 3s teleport");
            break;

        default:
            break;
    }

    GEngine->AddOnScreenDebugMessage(-1, 4.0f, FColor::Cyan, Message);
}

void UPhase1ViewModeSubsystem::Tick(const float DeltaSeconds)
{
    UWorld* World = GetWorld();
    if (!World || !World->IsGameWorld())
    {
        return;
    }

    APlayerController* PC = World->GetFirstPlayerController();
    if (!PC || !PC->GetPawn())
    {
        return;
    }

    if (!bModeInitialized)
    {
        TransitionToMode(CurrentMode, PC);
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

    if (FPhase1ModeNode* Active = GetActiveNode())
    {
        Active->Tick(PC, DeltaSeconds);
    }
}
