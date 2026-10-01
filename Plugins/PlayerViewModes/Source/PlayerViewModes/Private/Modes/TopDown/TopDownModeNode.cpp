#include "Modes/TopDown/TopDownModeNode.h"

#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "PlayerTopDownCamera.h"

namespace TopDownMode
{
    constexpr float OrbitYawDegreesPerMousePixel = 1.25f;
    constexpr float OrbitPitchDegreesPerMousePixel = 0.75f;
    constexpr float SprintPulseWindowSeconds = 0.45f;
}

FPlayerModeCapabilities UTopDownModeNode::GetCapabilities() const
{
    FPlayerModeCapabilities Caps;
    Caps.bCharacterMovement = true;
    Caps.bSprint = true;
    Caps.bLiveCasting = true;
    Caps.bSpellCreation = true;
    return Caps;
}

APlayerTopDownCamera* UTopDownModeNode::GetOrSpawnCamera(APlayerController* PC)
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
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    Camera = World->SpawnActor<APlayerTopDownCamera>(
        APlayerTopDownCamera::StaticClass(),
        FVector::ZeroVector,
        FRotator::ZeroRotator,
        Params);

    return Camera.Get();
}

void UTopDownModeNode::ApplyPresentation(APlayerController* PC)
{
    if (!PC)
    {
        return;
    }

    InputLock.Acquire(PC);
    PC->bShowMouseCursor = true;

    FInputModeGameAndUI Mode;
    Mode.SetHideCursorDuringCapture(false);
    Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    PC->SetInputMode(Mode);

    if (Camera.IsValid())
    {
        PC->SetViewTargetWithBlend(Camera.Get(), 0.20f);
    }
}

void UTopDownModeNode::Enter(APlayerController* PC)
{
    if (!PC || !PC->GetPawn())
    {
        return;
    }

    Character = Cast<ACharacter>(PC->GetPawn());
    OrbitYawDegrees = PC->GetControlRotation().Yaw;
    OrbitElevationDegrees = 62.0f;
    bSprinting = false;
    SprintWindowRemainingSeconds = 0.0f;
    bHaveDestination = false;

    if (ACharacter* Current = Character.Get())
    {
        PreviousJumpMaxCount = Current->JumpMaxCount;
        Current->JumpMaxCount = 0;

        bPreviousUseControllerRotationYaw = Current->bUseControllerRotationYaw;
        Current->bUseControllerRotationYaw = false;

        if (UCharacterMovementComponent* Movement = Current->GetCharacterMovement())
        {
            PreviousWalkSpeed = Movement->MaxWalkSpeed;
            bPreviousOrientRotationToMovement = Movement->bOrientRotationToMovement;
            Movement->bOrientRotationToMovement = true;
            Movement->StopMovementImmediately();
        }
    }

    APlayerTopDownCamera* CameraActor = GetOrSpawnCamera(PC);
    if (CameraActor)
    {
        CameraActor->SetOrbit(OrbitYawDegrees, OrbitElevationDegrees);
        UpdateCamera(PC);
    }

    ApplyPresentation(PC);
}

void UTopDownModeNode::Exit(APlayerController* PC)
{
    bHaveDestination = false;
    SprintWindowRemainingSeconds = 0.0f;
    SetSprint(false);

    if (ACharacter* Current = Character.Get())
    {
        Current->JumpMaxCount = PreviousJumpMaxCount;
        Current->bUseControllerRotationYaw = bPreviousUseControllerRotationYaw;

        if (UCharacterMovementComponent* Movement = Current->GetCharacterMovement())
        {
            Movement->bOrientRotationToMovement = bPreviousOrientRotationToMovement;
            Movement->MaxWalkSpeed = PreviousWalkSpeed;
        }
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

    Character.Reset();
}

void UTopDownModeNode::ResumeAfterExternalView(APlayerController* PC)
{
    ApplyPresentation(PC);
}

void UTopDownModeNode::SetSprint(const bool bEnabled)
{
    if (bSprinting == bEnabled)
    {
        return;
    }

    bSprinting = bEnabled;

    if (ACharacter* Current = Character.Get())
    {
        if (UCharacterMovementComponent* Movement = Current->GetCharacterMovement())
        {
            Movement->MaxWalkSpeed = PreviousWalkSpeed * (bEnabled ? 1.75f : 1.0f);
        }
    }
}

void UTopDownModeNode::UpdateCamera(APlayerController* PC)
{
    if (!PC || !PC->GetPawn() || !Camera.IsValid())
    {
        return;
    }

    Camera->FollowPosition(PC->GetPawn()->GetActorLocation());
    Camera->SetOrbit(OrbitYawDegrees, OrbitElevationDegrees);
}

bool UTopDownModeNode::FindCursorWorldPoint(APlayerController* PC, FVector& OutPoint) const
{
    if (!PC)
    {
        return false;
    }

    FHitResult Hit;
    if (PC->GetHitResultUnderCursorByChannel(
        UEngineTypes::ConvertToTraceType(ECC_Visibility), false, Hit) &&
        Hit.bBlockingHit &&
        Hit.GetActor() != PC->GetPawn())
    {
        OutPoint = Hit.ImpactPoint;
        return true;
    }

    FVector RayOrigin;
    FVector RayDirection;
    const APawn* Pawn = PC->GetPawn();

    if (!Pawn || !PC->DeprojectMousePositionToWorld(RayOrigin, RayDirection))
    {
        return false;
    }

    if (FMath::Abs(RayDirection.Z) < 0.001f)
    {
        return false;
    }

    const float T = (Pawn->GetActorLocation().Z - RayOrigin.Z) / RayDirection.Z;
    if (T < 0.0f || T > 100000.0f)
    {
        return false;
    }

    OutPoint = RayOrigin + RayDirection * T;
    return true;
}

void UTopDownModeNode::HandleInput(APlayerController* PC, const float DeltaSeconds)
{
    if (!PC)
    {
        return;
    }

    const float WheelAxis = PC->GetInputAnalogKeyState(EKeys::MouseWheelAxis);
    const bool bUpPulse =
        WheelAxis > 0.05f || PC->WasInputKeyJustPressed(EKeys::MouseScrollUp);
    const bool bDownPulse =
        WheelAxis < -0.05f || PC->WasInputKeyJustPressed(EKeys::MouseScrollDown);
    const bool bHoldingMove = PC->IsInputKeyDown(EKeys::RightMouseButton);

    if (!bHoldingMove || bDownPulse)
    {
        SprintWindowRemainingSeconds = 0.0f;
    }
    else if (bUpPulse)
    {
        SprintWindowRemainingSeconds = TopDownMode::SprintPulseWindowSeconds;
    }
    else
    {
        SprintWindowRemainingSeconds =
            FMath::Max(0.0f, SprintWindowRemainingSeconds - DeltaSeconds);
    }

    SetSprint(bHoldingMove && SprintWindowRemainingSeconds > 0.0f);

    if (PC->IsInputKeyDown(EKeys::LeftMouseButton))
    {
        float DeltaX = 0.0f;
        float DeltaY = 0.0f;
        PC->GetInputMouseDelta(DeltaX, DeltaY);

        OrbitYawDegrees = FRotator::NormalizeAxis(
            OrbitYawDegrees + DeltaX * TopDownMode::OrbitYawDegreesPerMousePixel);

        OrbitElevationDegrees = FMath::Clamp(
            OrbitElevationDegrees - DeltaY * TopDownMode::OrbitPitchDegreesPerMousePixel,
            16.0f,
            84.0f);
    }

    if (!bHoldingMove)
    {
        bHaveDestination = false;
    }
    else
    {
        FVector Point;
        if (FindCursorWorldPoint(PC, Point))
        {
            MoveDestination = Point;
            bHaveDestination = true;
        }
    }

    ACharacter* Current = Cast<ACharacter>(PC->GetPawn());
    if (Current && bHoldingMove && bHaveDestination)
    {
        FVector Delta = MoveDestination - Current->GetActorLocation();
        Delta.Z = 0.0f;

        if (Delta.SizeSquared() > FMath::Square(80.0f))
        {
            Current->AddMovementInput(Delta.GetSafeNormal(), 1.0f, true);
        }
        else if (UCharacterMovementComponent* Movement = Current->GetCharacterMovement())
        {
            Movement->StopMovementImmediately();
        }
    }
}

void UTopDownModeNode::TickMode(APlayerController* PC, const float DeltaSeconds)
{
    HandleInput(PC, DeltaSeconds);
    UpdateCamera(PC);
}

bool UTopDownModeNode::GetCastRay(FVector& OutOrigin, FVector& OutDirection) const
{
    ACharacter* Current = Character.Get();
    APlayerController* PC = Current ? Cast<APlayerController>(Current->GetController()) : nullptr;

    if (!Current || !PC)
    {
        return false;
    }

    OutOrigin = Current->GetActorLocation() + FVector(0.0f, 0.0f, 100.0f);

    FVector Point;
    if (FindCursorWorldPoint(PC, Point))
    {
        FVector ToPoint = Point - OutOrigin;
        ToPoint.Z = 0.0f;

        if (ToPoint.SizeSquared() > FMath::Square(75.0f))
        {
            OutDirection = ToPoint.GetSafeNormal();
            return true;
        }
    }

    OutDirection = FRotator(0.0f, OrbitYawDegrees, 0.0f).Vector();
    return true;
}
