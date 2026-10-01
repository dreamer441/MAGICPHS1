#pragma once

#include "CoreMinimal.h"
#include "Core/PlayerGameplayModeNode.h"
#include "Core/PulseSprintController.h"
#include "Widgets/SWidget.h"
#include "ThirdPersonModeNode.generated.h"

class USpringArmComponent;

/**
 * Third-person gameplay node.
 *
 * IMPORTANT:
 * This mode deliberately uses the character's native third-person camera rig.
 * We only tune its existing spring arm for RMB shoulder aim. This preserves the
 * template/controller camera rotation chain and normal movement behavior.
 */
UCLASS(NotBlueprintable)
class UThirdPersonModeNode final : public UPlayerGameplayModeNode
{
    GENERATED_BODY()

public:
    virtual EPlayerGameplayMode GetMode() const override
    {
        return EPlayerGameplayMode::ThirdPerson;
    }

    virtual FPlayerModeCapabilities GetCapabilities() const override;

    virtual void Enter(APlayerController* PC) override;
    virtual void Exit(APlayerController* PC) override;
    virtual void TickMode(APlayerController* PC, float DeltaSeconds) override;
    virtual void ResumeAfterExternalView(APlayerController* PC) override;

private:
    FPulseSprintController Sprint;

    TWeakObjectPtr<USpringArmComponent> NativeSpringArm;

    float NormalArmLength = 400.0f;
    FVector NormalSocketOffset = FVector::ZeroVector;
    FVector NormalTargetOffset = FVector::ZeroVector;
    bool bCapturedNativeCamera = false;

    bool bAimZoomed = false;
    TSharedPtr<SWidget> AimReticleWidget;

    void CaptureNativeCamera(APlayerController* PC);
    void RestoreNativeCamera();
    void ApplyPresentation(APlayerController* PC);
    void UpdateAimCamera(float DeltaSeconds);

    void SetAimZoomed(bool bEnabled);
    void ShowAimReticle();
    void HideAimReticle();
};
