#include "Modes/FirstPersonModeNode.h"

#include "Phase1Character.h"

FPhase1ModeCapabilities FFirstPersonModeNode::GetCapabilities() const
{
    FPhase1ModeCapabilities Caps;
    Caps.bCanCast = false;
    Caps.bLiveConstruction = false;
    Caps.bUsesWorldCursor = false;
    Caps.bDetachedCamera = false;
    return Caps;
}

void FFirstPersonModeNode::Enter(APlayerController* PC)
{
    SprintWindowSeconds = 0.0f;
    SetWorldCursorMode(PC, false);

    if (APhase1Character* Character = GetCharacter(PC))
    {
        Character->RestoreGroundMovement();
        Character->SetPhase1WalkSpeed(500.0f);
        Character->ConfigureFirstPersonCamera();
    }
}

void FFirstPersonModeNode::Exit(APlayerController* PC)
{
    SprintWindowSeconds = 0.0f;
}

void FFirstPersonModeNode::Tick(APlayerController* PC, const float DeltaSeconds)
{
    APhase1Character* Character = GetCharacter(PC);
    if (!PC || !Character)
    {
        return;
    }

    ApplyMouseLook(PC, 0.18f);
    ApplyCameraRelativeGroundMove(PC);
    ApplyJumpInput(PC);

    if (IsShiftJustPressed(PC))
    {
        SprintWindowSeconds = FMath::Min(SprintWindowSeconds + 0.70f, 1.50f);
    }

    SprintWindowSeconds = FMath::Max(0.0f, SprintWindowSeconds - DeltaSeconds);
    Character->SetPhase1WalkSpeed(SprintWindowSeconds > 0.0f ? 900.0f : 500.0f);
}
