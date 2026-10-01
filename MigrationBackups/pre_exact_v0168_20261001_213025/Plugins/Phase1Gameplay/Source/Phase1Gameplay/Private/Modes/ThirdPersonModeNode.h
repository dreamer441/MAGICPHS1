#pragma once

#include "Core/Phase1ModeNode.h"

class FThirdPersonModeNode final : public FPhase1ModeNode
{
public:
    EPhase1GameplayMode GetMode() const override { return EPhase1GameplayMode::ThirdPerson; }
    FPhase1ModeCapabilities GetCapabilities() const override;

    void Enter(APlayerController* PC) override;
    void Exit(APlayerController* PC) override;
    void Tick(APlayerController* PC, float DeltaSeconds) override;

private:
    float SprintWindowSeconds = 0.0f;
};
