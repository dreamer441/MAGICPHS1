#pragma once

#include "CoreMinimal.h"
#include "Core/PlayerGameplayModeNode.h"
#include "Core/PulseSprintController.h"
#include "FirstPersonModeNode.generated.h"

class APlayerFirstPersonCamera;

UCLASS(NotBlueprintable)
class UFirstPersonModeNode final : public UPlayerGameplayModeNode
{
    GENERATED_BODY()

public:
    virtual EPlayerGameplayMode GetMode() const override { return EPlayerGameplayMode::FirstPerson; }
    virtual FPlayerModeCapabilities GetCapabilities() const override;

    virtual void Enter(APlayerController* PC) override;
    virtual void Exit(APlayerController* PC) override;
    virtual void TickMode(APlayerController* PC, float DeltaSeconds) override;
    virtual void ResumeAfterExternalView(APlayerController* PC) override;

private:
    TWeakObjectPtr<APlayerFirstPersonCamera> Camera;
    FPulseSprintController Sprint;

    APlayerFirstPersonCamera* GetOrSpawnCamera(APlayerController* PC);
    void ApplyPresentation(APlayerController* PC);
};
