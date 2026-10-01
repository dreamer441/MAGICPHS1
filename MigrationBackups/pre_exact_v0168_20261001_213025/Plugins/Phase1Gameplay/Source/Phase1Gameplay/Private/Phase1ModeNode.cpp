#include "Core/Phase1ModeNode.h"

#include "Phase1Character.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"

bool FPhase1ModeNode::GetCastRay(
    APlayerController* PC,
    FVector& OutOrigin,
    FVector& OutDirection) const
{
    return CameraCenterRay(PC, OutOrigin, OutDirection);
}

bool FPhase1ModeNode::GetInteractionPoint(FVector& OutPoint) const
{
    OutPoint = FVector::ZeroVector;
    return false;
}

APhase1Character* FPhase1ModeNode::GetCharacter(APlayerController* PC) const
{
    return PC ? Cast<APhase1Character>(PC->GetPawn()) : nullptr;
}

void FPhase1ModeNode::ApplyMouseLook(
    APlayerController* PC,
    const float DegreesPerMouseUnit) const
{
    if (!PC)
    {
        return;
    }

    float MouseX = 0.0f;
    float MouseY = 0.0f;
    PC->GetInputMouseDelta(MouseX, MouseY);

    FRotator ControlRotation = PC->GetControlRotation();
    ControlRotation.Yaw += MouseX * DegreesPerMouseUnit;

    float Pitch = FRotator::NormalizeAxis(ControlRotation.Pitch);
    Pitch = FMath::Clamp(
        Pitch - MouseY * DegreesPerMouseUnit,
        -82.0f,
        82.0f);

    ControlRotation.Pitch = Pitch;
    ControlRotation.Roll = 0.0f;
    PC->SetControlRotation(ControlRotation);
}

void FPhase1ModeNode::ApplyCameraRelativeGroundMove(APlayerController* PC) const
{
    APhase1Character* Character = GetCharacter(PC);
    if (!PC || !Character)
    {
        return;
    }

    const FRotator YawOnly(0.0f, PC->GetControlRotation().Yaw, 0.0f);
    const FVector Forward = FRotationMatrix(YawOnly).GetUnitAxis(EAxis::X);
    const FVector Right = FRotationMatrix(YawOnly).GetUnitAxis(EAxis::Y);

    float ForwardScale = 0.0f;
    float RightScale = 0.0f;

    if (PC->IsInputKeyDown(EKeys::W)) ForwardScale += 1.0f;
    if (PC->IsInputKeyDown(EKeys::S)) ForwardScale -= 1.0f;
    if (PC->IsInputKeyDown(EKeys::D)) RightScale += 1.0f;
    if (PC->IsInputKeyDown(EKeys::A)) RightScale -= 1.0f;

    FVector Desired = Forward * ForwardScale + Right * RightScale;
    if (!Desired.IsNearlyZero())
    {
        Desired.Normalize();
        Character->AddMovementInput(Desired, 1.0f, true);
    }
}

void FPhase1ModeNode::ApplyJumpInput(APlayerController* PC) const
{
    APhase1Character* Character = GetCharacter(PC);
    if (!PC || !Character)
    {
        return;
    }

    if (PC->WasInputKeyJustPressed(EKeys::SpaceBar))
    {
        Character->Jump();
    }

    if (PC->WasInputKeyJustReleased(EKeys::SpaceBar))
    {
        Character->StopJumping();
    }
}

bool FPhase1ModeNode::CursorPointOnHorizontalPlane(
    APlayerController* PC,
    const float PlaneZ,
    FVector& OutPoint) const
{
    if (!PC)
    {
        return false;
    }

    FVector RayOrigin;
    FVector RayDirection;
    if (!PC->DeprojectMousePositionToWorld(RayOrigin, RayDirection))
    {
        return false;
    }

    if (FMath::Abs(RayDirection.Z) <= KINDA_SMALL_NUMBER)
    {
        return false;
    }

    const float T = (PlaneZ - RayOrigin.Z) / RayDirection.Z;
    if (T <= 0.0f)
    {
        return false;
    }

    OutPoint = RayOrigin + RayDirection * T;
    return true;
}

bool FPhase1ModeNode::CameraCenterRay(
    APlayerController* PC,
    FVector& OutOrigin,
    FVector& OutDirection) const
{
    if (!PC)
    {
        return false;
    }

    FRotator ViewRotation;
    PC->GetPlayerViewPoint(OutOrigin, ViewRotation);
    OutDirection = ViewRotation.Vector().GetSafeNormal();
    return !OutDirection.IsNearlyZero();
}

void FPhase1ModeNode::SetWorldCursorMode(
    APlayerController* PC,
    const bool bEnabled) const
{
    if (!PC)
    {
        return;
    }

    PC->bShowMouseCursor = bEnabled;
    PC->bEnableClickEvents = bEnabled;
    PC->bEnableMouseOverEvents = bEnabled;
}

bool FPhase1ModeNode::IsShiftJustPressed(APlayerController* PC) const
{
    return PC &&
        (PC->WasInputKeyJustPressed(EKeys::LeftShift) ||
         PC->WasInputKeyJustPressed(EKeys::RightShift));
}
