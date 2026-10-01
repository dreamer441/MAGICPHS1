#include "Modes/TopDownModeNode.h"

#include "Phase1Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"

FPhase1ModeCapabilities FTopDownModeNode::GetCapabilities() const
{
    FPhase1ModeCapabilities Caps;
    Caps.bCanCast = true;
    Caps.bLiveConstruction = true;
    Caps.bUsesWorldCursor = true;
    Caps.bDetachedCamera = false;
    return Caps;
}

void FTopDownModeNode::Enter(APlayerController* PC)
{
    SprintWindowSeconds = 0.0f;
    bHaveDestination = false;
    SetWorldCursorMode(PC, true);

    if (APhase1Character* Character = GetCharacter(PC))
    {
        Character->RestoreGroundMovement();
        Character->SetPhase1WalkSpeed(480.0f);
        Character->ConfigureTopDownCamera(OrbitYawDegrees);
    }
}

void FTopDownModeNode::Exit(APlayerController* PC)
{
    SprintWindowSeconds = 0.0f;
    bHaveDestination = false;
    SetWorldCursorMode(PC, false);

    if (APhase1Character* Character = GetCharacter(PC))
    {
        if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
        {
            Movement->StopMovementImmediately();
        }
    }
}

void FTopDownModeNode::Tick(APlayerController* PC, const float DeltaSeconds)
{
    APhase1Character* Character = GetCharacter(PC);
    if (!PC || !Character)
    {
        return;
    }

    if (PC->IsInputKeyDown(EKeys::LeftMouseButton))
    {
        float MouseX = 0.0f;
        float MouseY = 0.0f;
        PC->GetInputMouseDelta(MouseX, MouseY);
        OrbitYawDegrees += MouseX * 0.30f;
        Character->ConfigureTopDownCamera(OrbitYawDegrees);
    }

    if (PC->IsInputKeyDown(EKeys::RightMouseButton))
    {
        FVector CursorPoint;
        if (CursorPointOnHorizontalPlane(PC, Character->GetActorLocation().Z, CursorPoint))
        {
            Destination = CursorPoint;
            bHaveDestination = true;
        }
    }

    if (PC->WasInputKeyJustPressed(EKeys::MouseScrollUp))
    {
        SprintWindowSeconds = 0.75f;
    }

    SprintWindowSeconds = FMath::Max(0.0f, SprintWindowSeconds - DeltaSeconds);
    Character->SetPhase1WalkSpeed(SprintWindowSeconds > 0.0f ? 820.0f : 480.0f);

    if (bHaveDestination)
    {
        FVector Delta = Destination - Character->GetActorLocation();
        Delta.Z = 0.0f;

        if (Delta.SizeSquared() <= FMath::Square(80.0f))
        {
            bHaveDestination = false;
            if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
            {
                Movement->StopMovementImmediately();
            }
        }
        else
        {
            Character->AddMovementInput(Delta.GetSafeNormal(), 1.0f, true);
        }
    }
}

bool FTopDownModeNode::GetCastRay(
    APlayerController* PC,
    FVector& OutOrigin,
    FVector& OutDirection) const
{
    APhase1Character* Character = GetCharacter(PC);
    if (!PC || !Character)
    {
        return false;
    }

    FVector CursorPoint;
    if (!CursorPointOnHorizontalPlane(PC, Character->GetActorLocation().Z, CursorPoint))
    {
        return false;
    }

    OutOrigin = Character->GetActorLocation() + FVector(0.0f, 0.0f, 65.0f);
    OutDirection = (CursorPoint - OutOrigin).GetSafeNormal();
    return !OutDirection.IsNearlyZero();
}
