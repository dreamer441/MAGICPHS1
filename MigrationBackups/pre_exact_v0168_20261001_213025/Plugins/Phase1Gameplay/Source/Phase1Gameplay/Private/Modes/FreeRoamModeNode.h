#pragma once

#include "Core/Phase1ModeNode.h"

class ACameraActor;

class FFreeRoamModeNode final : public FPhase1ModeNode
{
public:
    EPhase1GameplayMode GetMode() const override { return EPhase1GameplayMode::FreeRoam; }
    FPhase1ModeCapabilities GetCapabilities() const override;

    void Enter(APlayerController* PC) override;
    void Exit(APlayerController* PC) override;
    void Tick(APlayerController* PC, float DeltaSeconds) override;

    bool GetCastRay(APlayerController* PC, FVector& OutOrigin, FVector& OutDirection) const override;
    bool GetInteractionPoint(FVector& OutPoint) const override;

private:
    TWeakObjectPtr<ACameraActor> FreeCamera;

    float RightMouseHoldSeconds = 0.0f;
    bool bTeleportTriggered = false;

    bool bHaveBodyDestination = false;
    FVector BodyDestination = FVector::ZeroVector;

    bool bHaveInteractionPoint = false;
    FVector LastInteractionPoint = FVector::ZeroVector;
};
