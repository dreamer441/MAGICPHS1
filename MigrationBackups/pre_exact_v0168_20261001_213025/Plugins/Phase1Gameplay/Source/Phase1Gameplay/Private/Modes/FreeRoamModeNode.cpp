#include "Modes/FreeRoamModeNode.h"

#include "Phase1Character.h"
#include "Camera/CameraActor.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"

FPhase1ModeCapabilities FFreeRoamModeNode::GetCapabilities() const
{
    FPhase1ModeCapabilities Caps;
    Caps.bCanCast = true;
    Caps.bLiveConstruction = false;
    Caps.bUsesWorldCursor = true;
    Caps.bDetachedCamera = true;
    return Caps;
}

void FFreeRoamModeNode::Enter(APlayerController* PC)
{
    APhase1Character* Character = GetCharacter(PC);
    if (!PC || !Character || !PC->GetWorld())
    {
        return;
    }

    SetWorldCursorMode(PC, true);

    FVector ViewLocation;
    FRotator ViewRotation;
    PC->GetPlayerViewPoint(ViewLocation, ViewRotation);

    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    ACameraActor* Camera = PC->GetWorld()->SpawnActor<ACameraActor>(
        ACameraActor::StaticClass(),
        ViewLocation,
        ViewRotation,
        SpawnParams);

    FreeCamera = Camera;
    if (Camera)
    {
        PC->SetViewTarget(Camera);
    }

    RightMouseHoldSeconds = 0.0f;
    bTeleportTriggered = false;
    bHaveBodyDestination = false;
    bHaveInteractionPoint = false;
}

void FFreeRoamModeNode::Exit(APlayerController* PC)
{
    if (PC)
    {
        if (APhase1Character* Character = GetCharacter(PC))
        {
            PC->SetViewTarget(Character);
        }
    }

    if (ACameraActor* Camera = FreeCamera.Get())
    {
        Camera->Destroy();
    }

    FreeCamera.Reset();
    SetWorldCursorMode(PC, false);

    RightMouseHoldSeconds = 0.0f;
    bTeleportTriggered = false;
    bHaveBodyDestination = false;
    bHaveInteractionPoint = false;
}

void FFreeRoamModeNode::Tick(APlayerController* PC, const float DeltaSeconds)
{
    APhase1Character* Character = GetCharacter(PC);
    ACameraActor* Camera = FreeCamera.Get();
    if (!PC || !Character || !Camera)
    {
        return;
    }

    FRotator CameraRotation = Camera->GetActorRotation();

    if (PC->IsInputKeyDown(EKeys::LeftMouseButton))
    {
        float MouseX = 0.0f;
        float MouseY = 0.0f;
        PC->GetInputMouseDelta(MouseX, MouseY);

        CameraRotation.Yaw += MouseX * 0.20f;
        CameraRotation.Pitch = FMath::Clamp(
            FRotator::NormalizeAxis(CameraRotation.Pitch) - MouseY * 0.20f,
            -82.0f,
            82.0f);
        CameraRotation.Roll = 0.0f;
        Camera->SetActorRotation(CameraRotation);
    }

    FVector Move = FVector::ZeroVector;
    const FVector Forward = CameraRotation.Vector().GetSafeNormal();
    const FVector Right = FRotationMatrix(CameraRotation).GetUnitAxis(EAxis::Y);

    if (PC->IsInputKeyDown(EKeys::W)) Move += Forward;
    if (PC->IsInputKeyDown(EKeys::S)) Move -= Forward;
    if (PC->IsInputKeyDown(EKeys::D)) Move += Right;
    if (PC->IsInputKeyDown(EKeys::A)) Move -= Right;
    if (PC->IsInputKeyDown(EKeys::E)) Move += FVector::UpVector;
    if (PC->IsInputKeyDown(EKeys::Q)) Move -= FVector::UpVector;

    if (!Move.IsNearlyZero())
    {
        const bool bFast =
            PC->IsInputKeyDown(EKeys::LeftShift) ||
            PC->IsInputKeyDown(EKeys::RightShift);

        const float Speed = bFast ? 1800.0f : 900.0f;
        Camera->AddActorWorldOffset(Move.GetSafeNormal() * Speed * DeltaSeconds);
    }

    const bool bRightDown = PC->IsInputKeyDown(EKeys::RightMouseButton);

    if (PC->WasInputKeyJustPressed(EKeys::RightMouseButton))
    {
        RightMouseHoldSeconds = 0.0f;
        bTeleportTriggered = false;
    }

    if (bRightDown)
    {
        RightMouseHoldSeconds += DeltaSeconds;

        FVector CursorPoint;
        if (CursorPointOnHorizontalPlane(PC, Character->GetActorLocation().Z, CursorPoint))
        {
            LastInteractionPoint = CursorPoint;
            bHaveInteractionPoint = true;
        }

        if (RightMouseHoldSeconds >= 3.0f &&
            !bTeleportTriggered &&
            bHaveInteractionPoint)
        {
            Character->SetActorLocation(
                LastInteractionPoint + FVector(0.0f, 0.0f, 4.0f),
                false,
                nullptr,
                ETeleportType::TeleportPhysics);

            if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
            {
                Movement->StopMovementImmediately();
            }

            bHaveBodyDestination = false;
            bTeleportTriggered = true;
        }
    }

    if (PC->WasInputKeyJustReleased(EKeys::RightMouseButton))
    {
        if (!bTeleportTriggered && bHaveInteractionPoint)
        {
            BodyDestination = LastInteractionPoint;
            bHaveBodyDestination = true;
        }

        RightMouseHoldSeconds = 0.0f;
        bTeleportTriggered = false;
    }

    if (bHaveBodyDestination)
    {
        FVector Delta = BodyDestination - Character->GetActorLocation();
        Delta.Z = 0.0f;

        if (Delta.SizeSquared() <= FMath::Square(80.0f))
        {
            bHaveBodyDestination = false;
            if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
            {
                Movement->StopMovementImmediately();
            }
        }
        else
        {
            Character->SetPhase1WalkSpeed(520.0f);
            Character->AddMovementInput(Delta.GetSafeNormal(), 1.0f, true);
        }
    }
}

bool FFreeRoamModeNode::GetCastRay(
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

bool FFreeRoamModeNode::GetInteractionPoint(FVector& OutPoint) const
{
    if (!bHaveInteractionPoint)
    {
        return false;
    }

    OutPoint = LastInteractionPoint;
    return true;
}
