#pragma once

#include "Core/Phase1ModeNode.h"

class FTopDownModeNode final : public FPhase1ModeNode
{
public:
    EPhase1GameplayMode GetMode() const override { return EPhase1GameplayMode::TopDown; }
    FPhase1ModeCapabilities GetCapabilities() const override;

    void Enter(APlayerController* PC) override;
    void Exit(APlayerController* PC) override;
    void Tick(APlayerController* PC, float DeltaSeconds) override;

    bool GetCastRay(APlayerController* PC, FVector& OutOrigin, FVector& OutDirection) const override;

private:
    float OrbitYawDegrees = 35.0f;
    float SprintWindowSeconds = 0.0f;
    bool bHaveDestination = false;
    FVector Destination = FVector::ZeroVector;
};
