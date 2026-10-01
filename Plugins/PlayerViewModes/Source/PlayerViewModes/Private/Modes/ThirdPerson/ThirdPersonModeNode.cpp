#include "Modes/ThirdPerson/ThirdPersonModeNode.h"

#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputCoreTypes.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

namespace ThirdPersonAimTuning
{
    // Close shoulder view while RMB is held.
    constexpr float AimArmLength = 165.0f;

    // Added to the character's EXISTING socket offset instead of replacing it.
    constexpr float AimShoulderOffsetY = 72.0f;
    constexpr float AimShoulderOffsetZ = 8.0f;

    constexpr float ZoomInterpSpeed = 11.0f;
    constexpr float ShoulderInterpSpeed = 12.0f;
}

FPlayerModeCapabilities UThirdPersonModeNode::GetCapabilities() const
{
    FPlayerModeCapabilities Caps;
    Caps.bCharacterMovement = true;
    Caps.bJump = true;
    Caps.bSprint = true;
    return Caps;
}

void UThirdPersonModeNode::CaptureNativeCamera(APlayerController* PC)
{
    NativeSpringArm.Reset();
    bCapturedNativeCamera = false;

    ACharacter* Character =
        PC ? Cast<ACharacter>(PC->GetPawn()) : nullptr;

    if (!Character)
    {
        return;
    }

    if (USpringArmComponent* SpringArm =
        Character->FindComponentByClass<USpringArmComponent>())
    {
        NativeSpringArm = SpringArm;
        NormalArmLength = SpringArm->TargetArmLength;
        NormalSocketOffset = SpringArm->SocketOffset;
        NormalTargetOffset = SpringArm->TargetOffset;
        bCapturedNativeCamera = true;
    }
}

void UThirdPersonModeNode::RestoreNativeCamera()
{
    if (bCapturedNativeCamera)
    {
        if (USpringArmComponent* SpringArm =
            NativeSpringArm.Get())
        {
            SpringArm->TargetArmLength = NormalArmLength;
            SpringArm->SocketOffset = NormalSocketOffset;
            SpringArm->TargetOffset = NormalTargetOffset;
        }
    }

    NativeSpringArm.Reset();
    bCapturedNativeCamera = false;
}

void UThirdPersonModeNode::ShowAimReticle()
{
    if (AimReticleWidget.IsValid() ||
        !GEngine ||
        !GEngine->GameViewport)
    {
        return;
    }

    AimReticleWidget =
        SNew(SOverlay)

        + SOverlay::Slot()
        .HAlign(HAlign_Center)
        .VAlign(VAlign_Center)
        [
            SNew(SBox)
            .WidthOverride(18.0f)
            .HeightOverride(18.0f)
            [
                SNew(STextBlock)
                .Text(FText::FromString(TEXT("•")))
                .Justification(ETextJustify::Center)
                .Font(FCoreStyle::GetDefaultFontStyle(
                    "Bold",
                    20))
            ]
        ];

    GEngine->GameViewport->AddViewportWidgetContent(
        AimReticleWidget.ToSharedRef(),
        1000);
}

void UThirdPersonModeNode::HideAimReticle()
{
    if (!AimReticleWidget.IsValid())
    {
        return;
    }

    if (GEngine && GEngine->GameViewport)
    {
        GEngine->GameViewport->RemoveViewportWidgetContent(
            AimReticleWidget.ToSharedRef());
    }

    AimReticleWidget.Reset();
}

void UThirdPersonModeNode::SetAimZoomed(
    const bool bEnabled)
{
    if (bAimZoomed == bEnabled)
    {
        return;
    }

    bAimZoomed = bEnabled;

    if (bAimZoomed)
    {
        ShowAimReticle();
    }
    else
    {
        HideAimReticle();
    }
}

void UThirdPersonModeNode::ApplyPresentation(
    APlayerController* PC)
{
    if (!PC)
    {
        return;
    }

    PC->bShowMouseCursor = false;
    PC->SetInputMode(FInputModeGameOnly());

    // Critical fix:
    // Return view ownership to the possessed character. The character's native
    // camera + spring arm already follow PlayerController control rotation, so
    // mouse look and movement direction remain synchronized.
    if (APawn* Pawn = PC->GetPawn())
    {
        PC->SetViewTargetWithBlend(
            Pawn,
            0.15f);
    }
}

void UThirdPersonModeNode::Enter(
    APlayerController* PC)
{
    Sprint.Enter(
        PC ? Cast<ACharacter>(PC->GetPawn()) : nullptr);

    bAimZoomed = false;
    HideAimReticle();

    CaptureNativeCamera(PC);
    ApplyPresentation(PC);
}

void UThirdPersonModeNode::Exit(
    APlayerController* PC)
{
    Sprint.Exit();

    SetAimZoomed(false);
    RestoreNativeCamera();
}

void UThirdPersonModeNode::UpdateAimCamera(
    const float DeltaSeconds)
{
    USpringArmComponent* SpringArm =
        NativeSpringArm.Get();

    if (!SpringArm || !bCapturedNativeCamera)
    {
        return;
    }

    const float TargetArmLength =
        bAimZoomed
        ? ThirdPersonAimTuning::AimArmLength
        : NormalArmLength;

    FVector TargetSocketOffset =
        NormalSocketOffset;

    if (bAimZoomed)
    {
        TargetSocketOffset.Y +=
            ThirdPersonAimTuning::AimShoulderOffsetY;

        TargetSocketOffset.Z +=
            ThirdPersonAimTuning::AimShoulderOffsetZ;
    }

    SpringArm->TargetArmLength =
        FMath::FInterpTo(
            SpringArm->TargetArmLength,
            TargetArmLength,
            DeltaSeconds,
            ThirdPersonAimTuning::ZoomInterpSpeed);

    SpringArm->SocketOffset =
        FMath::VInterpTo(
            SpringArm->SocketOffset,
            TargetSocketOffset,
            DeltaSeconds,
            ThirdPersonAimTuning::ShoulderInterpSpeed);
}

void UThirdPersonModeNode::TickMode(
    APlayerController* PC,
    const float DeltaSeconds)
{
    Sprint.Tick(PC, DeltaSeconds);

    if (!PC)
    {
        return;
    }

    SetAimZoomed(
        PC->IsInputKeyDown(
            EKeys::RightMouseButton));

    UpdateAimCamera(DeltaSeconds);
}

void UThirdPersonModeNode::ResumeAfterExternalView(
    APlayerController* PC)
{
    // InnerRealm may temporarily replace the view target. Restore the native
    // possessed-pawn view without rebuilding the camera rig.
    ApplyPresentation(PC);

    if (!bCapturedNativeCamera)
    {
        CaptureNativeCamera(PC);
    }

    if (bAimZoomed)
    {
        ShowAimReticle();
    }
}
