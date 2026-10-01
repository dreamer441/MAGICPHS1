#include "Modes/FreeRoam/FreeRoamModeNode.h"

#include "Camera/PlayerCameraManager.h"
#include "Components/CapsuleComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "PlayerFreeRoamCamera.h"

namespace FreeRoamMode
{
    constexpr float CameraSpeed = 1500.0f;
    constexpr float LookDegreesPerMousePixel = 0.306f;
    constexpr float BodyArrivalRadius = 100.0f;

    // V016.2 radius was 55 cm. V016.3 doubles it.
    constexpr float PointerIndicatorRadius = 110.0f;
    constexpr float PointerIndicatorHeight = 4.0f;
    constexpr int32 PointerIndicatorSegments = 48;
    constexpr float PointerIndicatorThickness = 3.0f;

    constexpr float TeleportHoldRequiredSeconds = 3.0f;
    constexpr float TeleportAnchorToleranceCm = 150.0f;
}

FPlayerModeCapabilities UFreeRoamModeNode::GetCapabilities() const
{
    FPlayerModeCapabilities Caps;
    Caps.bWorldInteraction = true;
    Caps.bDetachedCamera = true;
    Caps.bLiveCasting = false;
    return Caps;
}

APlayerFreeRoamCamera* UFreeRoamModeNode::GetOrSpawnCamera(APlayerController* PC)
{
    if (Camera.IsValid())
    {
        return Camera.Get();
    }

    UWorld* World = PC ? PC->GetWorld() : nullptr;
    if (!World)
    {
        return nullptr;
    }

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride =
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    Camera = World->SpawnActor<APlayerFreeRoamCamera>(
        APlayerFreeRoamCamera::StaticClass(),
        FVector::ZeroVector,
        FRotator::ZeroRotator,
        Params);

    return Camera.Get();
}

void UFreeRoamModeNode::ApplyPresentation(APlayerController* PC)
{
    if (!PC)
    {
        return;
    }

    InputLock.Acquire(PC);
    PC->bShowMouseCursor = true;

    FInputModeGameAndUI Mode;
    Mode.SetHideCursorDuringCapture(false);
    Mode.SetLockMouseToViewportBehavior(
        EMouseLockMode::DoNotLock);
    PC->SetInputMode(Mode);

    if (Camera.IsValid())
    {
        PC->SetViewTargetWithBlend(
            Camera.Get(),
            0.20f);
    }
}

void UFreeRoamModeNode::Enter(APlayerController* PC)
{
    if (!PC)
    {
        return;
    }

    Body = Cast<ACharacter>(PC->GetPawn());
    bHavePointerPoint = false;
    bHaveInteractionPoint = false;
    bHaveBodyDestination = false;

    RightHoldSeconds = 0.0f;
    bRightHoldTriggeredTeleport = false;
    bRightHoldActive = false;

    FVector StartLocation =
        PC->GetPawn()
        ? PC->GetPawn()->GetActorLocation()
        : FVector::ZeroVector;

    FRotator StartRotation =
        PC->GetControlRotation();

    if (PC->PlayerCameraManager)
    {
        StartLocation =
            PC->PlayerCameraManager->GetCameraLocation();
        StartRotation =
            PC->PlayerCameraManager->GetCameraRotation();
    }

    if (APlayerFreeRoamCamera* CameraActor =
        GetOrSpawnCamera(PC))
    {
        CameraActor->SetSpatialView(
            StartLocation,
            StartRotation);
    }

    if (ACharacter* CurrentBody = Body.Get())
    {
        PreviousJumpMaxCount =
            CurrentBody->JumpMaxCount;

        CurrentBody->JumpMaxCount = 0;
        CurrentBody->StopJumping();
    }

    ApplyPresentation(PC);
}

void UFreeRoamModeNode::Exit(APlayerController* PC)
{
    bHaveBodyDestination = false;
    bHavePointerPoint = false;

    RightHoldSeconds = 0.0f;
    bRightHoldTriggeredTeleport = false;
    bRightHoldActive = false;

    if (ACharacter* CurrentBody = Body.Get())
    {
        CurrentBody->JumpMaxCount =
            PreviousJumpMaxCount;

        CurrentBody->StopJumping();
    }

    InputLock.Release();

    if (PC)
    {
        PC->bShowMouseCursor = false;
        PC->SetInputMode(FInputModeGameOnly());
    }

    if (Camera.IsValid())
    {
        Camera->Destroy();
        Camera.Reset();
    }

    Body.Reset();
}

void UFreeRoamModeNode::ResumeAfterExternalView(APlayerController* PC)
{
    ApplyPresentation(PC);
}

void UFreeRoamModeNode::HandleCameraInput(
    APlayerController* PC,
    const float DeltaSeconds)
{
    APlayerFreeRoamCamera* CameraActor =
        Camera.Get();

    if (!PC || !CameraActor)
    {
        return;
    }

    if (PC->IsInputKeyDown(EKeys::LeftMouseButton))
    {
        float DeltaX = 0.0f;
        float DeltaY = 0.0f;

        PC->GetInputMouseDelta(
            DeltaX,
            DeltaY);

        CameraActor->AddLookDelta(
            DeltaX * FreeRoamMode::LookDegreesPerMousePixel,
            -DeltaY * FreeRoamMode::LookDegreesPerMousePixel);
    }

    FVector Direction = FVector::ZeroVector;
    const FVector Forward =
        CameraActor->GetActorForwardVector();
    const FVector Right =
        CameraActor->GetActorRightVector();

    if (PC->IsInputKeyDown(EKeys::W)) Direction += Forward;
    if (PC->IsInputKeyDown(EKeys::S)) Direction -= Forward;
    if (PC->IsInputKeyDown(EKeys::D)) Direction += Right;
    if (PC->IsInputKeyDown(EKeys::A)) Direction -= Right;
    if (PC->IsInputKeyDown(EKeys::E)) Direction += FVector::UpVector;
    if (PC->IsInputKeyDown(EKeys::Q)) Direction -= FVector::UpVector;

    if (!Direction.IsNearlyZero())
    {
        CameraActor->AddSpatialOffset(
            Direction.GetSafeNormal() *
            FreeRoamMode::CameraSpeed *
            DeltaSeconds);
    }
}

bool UFreeRoamModeNode::FindWorldPoint(
    APlayerController* PC,
    FVector& OutPoint) const
{
    if (!PC)
    {
        return false;
    }

    FHitResult Hit;

    if (PC->GetHitResultUnderCursorByChannel(
        UEngineTypes::ConvertToTraceType(ECC_Visibility),
        false,
        Hit) &&
        Hit.bBlockingHit &&
        Hit.GetActor() != PC->GetPawn())
    {
        OutPoint = Hit.ImpactPoint;
        return true;
    }

    FVector RayOrigin;
    FVector RayDirection;
    const ACharacter* CurrentBody = Body.Get();

    if (!CurrentBody ||
        !PC->DeprojectMousePositionToWorld(
            RayOrigin,
            RayDirection))
    {
        return false;
    }

    if (FMath::Abs(RayDirection.Z) < 0.001f)
    {
        return false;
    }

    const float PlaneZ =
        CurrentBody->GetActorLocation().Z;

    const float T =
        (PlaneZ - RayOrigin.Z) /
        RayDirection.Z;

    if (T < 0.0f || T > 500000.0f)
    {
        return false;
    }

    OutPoint =
        RayOrigin + RayDirection * T;

    return true;
}

void UFreeRoamModeNode::UpdatePointerTarget(
    APlayerController* PC)
{
    FVector Point;

    if (FindWorldPoint(PC, Point))
    {
        PointerPoint = Point;
        bHavePointerPoint = true;
    }
    else
    {
        bHavePointerPoint = false;
    }
}

void UFreeRoamModeNode::DrawPointerIndicator() const
{
    UWorld* World = GetWorld();

    if (!World || !bHavePointerPoint)
    {
        return;
    }

    const FVector Center =
        PointerPoint +
        FVector(
            0.0f,
            0.0f,
            FreeRoamMode::PointerIndicatorHeight);

    for (int32 Index = 0;
         Index < FreeRoamMode::PointerIndicatorSegments;
         ++Index)
    {
        const float A0 =
            (2.0f * PI * static_cast<float>(Index)) /
            static_cast<float>(
                FreeRoamMode::PointerIndicatorSegments);

        const float A1 =
            (2.0f * PI * static_cast<float>(Index + 1)) /
            static_cast<float>(
                FreeRoamMode::PointerIndicatorSegments);

        const FVector P0 =
            Center +
            FVector(
                FMath::Cos(A0) *
                    FreeRoamMode::PointerIndicatorRadius,
                FMath::Sin(A0) *
                    FreeRoamMode::PointerIndicatorRadius,
                0.0f);

        const FVector P1 =
            Center +
            FVector(
                FMath::Cos(A1) *
                    FreeRoamMode::PointerIndicatorRadius,
                FMath::Sin(A1) *
                    FreeRoamMode::PointerIndicatorRadius,
                0.0f);

        DrawDebugLine(
            World,
            P0,
            P1,
            FColor::Cyan,
            false,
            0.0f,
            0,
            FreeRoamMode::PointerIndicatorThickness);
    }
}

void UFreeRoamModeNode::TeleportBodyTo(
    const FVector& WorldPoint)
{
    ACharacter* CurrentBody = Body.Get();

    if (!CurrentBody)
    {
        return;
    }

    float CapsuleHalfHeight = 90.0f;

    if (const UCapsuleComponent* Capsule =
        CurrentBody->GetCapsuleComponent())
    {
        CapsuleHalfHeight =
            Capsule->GetScaledCapsuleHalfHeight();
    }

    const FVector Destination =
        WorldPoint +
        FVector(
            0.0f,
            0.0f,
            CapsuleHalfHeight + 2.0f);

    if (UCharacterMovementComponent* Movement =
        CurrentBody->GetCharacterMovement())
    {
        Movement->StopMovementImmediately();
    }

    bHaveBodyDestination = false;

    CurrentBody->SetActorLocation(
        Destination,
        false,
        nullptr,
        ETeleportType::TeleportPhysics);

    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(
            -1,
            1.5f,
            FColor::Cyan,
            TEXT("FREE ROAM: body teleported"));
    }
}

void UFreeRoamModeNode::HandleRightMouseAction(
    APlayerController* PC,
    const float DeltaSeconds)
{
    if (!PC)
    {
        return;
    }

    if (PC->WasInputKeyJustPressed(
        EKeys::RightMouseButton))
    {
        RightHoldSeconds = 0.0f;
        bRightHoldTriggeredTeleport = false;
        bRightHoldActive = bHavePointerPoint;

        if (bHavePointerPoint)
        {
            RightHoldAnchorPoint =
                PointerPoint;
        }
    }

    if (bRightHoldActive &&
        PC->IsInputKeyDown(EKeys::RightMouseButton))
    {
        if (!bHavePointerPoint)
        {
            RightHoldSeconds = 0.0f;
            return;
        }

        const bool bPointerMovedTooFar =
            FVector::DistSquared2D(
                PointerPoint,
                RightHoldAnchorPoint) >
            FMath::Square(
                FreeRoamMode::TeleportAnchorToleranceCm);

        if (bPointerMovedTooFar)
        {
            RightHoldAnchorPoint =
                PointerPoint;
            RightHoldSeconds = 0.0f;
        }
        else
        {
            RightHoldSeconds +=
                FMath::Max(
                    DeltaSeconds,
                    0.0f);
        }

        if (!bRightHoldTriggeredTeleport &&
            RightHoldSeconds >=
                FreeRoamMode::TeleportHoldRequiredSeconds)
        {
            InteractionPoint =
                PointerPoint;
            bHaveInteractionPoint = true;

            TeleportBodyTo(PointerPoint);
            bRightHoldTriggeredTeleport = true;
        }
    }

    if (bRightHoldActive &&
        PC->WasInputKeyJustReleased(
            EKeys::RightMouseButton))
    {
        if (!bRightHoldTriggeredTeleport &&
            bHavePointerPoint)
        {
            // Quick RMB preserves the original free-roam body command.
            InteractionPoint =
                PointerPoint;
            bHaveInteractionPoint = true;

            BodyDestination =
                PointerPoint;
            bHaveBodyDestination = true;
        }

        RightHoldSeconds = 0.0f;
        bRightHoldTriggeredTeleport = false;
        bRightHoldActive = false;
    }
}

void UFreeRoamModeNode::UpdateBodyMovement()
{
    ACharacter* CurrentBody = Body.Get();

    if (!CurrentBody || !bHaveBodyDestination)
    {
        return;
    }

    FVector Delta =
        BodyDestination -
        CurrentBody->GetActorLocation();

    Delta.Z = 0.0f;

    if (Delta.SizeSquared() >
        FMath::Square(
            FreeRoamMode::BodyArrivalRadius))
    {
        CurrentBody->AddMovementInput(
            Delta.GetSafeNormal(),
            1.0f,
            true);
    }
    else
    {
        bHaveBodyDestination = false;

        if (UCharacterMovementComponent* Movement =
            CurrentBody->GetCharacterMovement())
        {
            Movement->StopMovementImmediately();
        }
    }
}

void UFreeRoamModeNode::TickMode(
    APlayerController* PC,
    const float DeltaSeconds)
{
    if (ACharacter* CurrentBody = Body.Get())
    {
        CurrentBody->JumpMaxCount = 0;

        if (PC &&
            PC->WasInputKeyJustPressed(
                EKeys::SpaceBar))
        {
            CurrentBody->StopJumping();
        }
    }

    HandleCameraInput(
        PC,
        DeltaSeconds);

    UpdatePointerTarget(PC);
    DrawPointerIndicator();

    HandleRightMouseAction(
        PC,
        DeltaSeconds);

    UpdateBodyMovement();
}

bool UFreeRoamModeNode::GetCastRay(
    FVector& OutOrigin,
    FVector& OutDirection) const
{
    const ACharacter* CurrentBody =
        Body.Get();

    if (!CurrentBody ||
        !bHavePointerPoint)
    {
        return false;
    }

    OutOrigin =
        CurrentBody->GetActorLocation() +
        FVector(0.0f, 0.0f, 100.0f);

    FVector ToPointer =
        PointerPoint - OutOrigin;

    ToPointer.Z = 0.0f;

    if (ToPointer.SizeSquared() <=
        FMath::Square(25.0f))
    {
        return false;
    }

    OutDirection =
        ToPointer.GetSafeNormal();

    return true;
}

bool UFreeRoamModeNode::GetInteractionPoint(
    FVector& OutPoint) const
{
    if (!bHaveInteractionPoint)
    {
        return false;
    }

    OutPoint =
        InteractionPoint;

    return true;
}
