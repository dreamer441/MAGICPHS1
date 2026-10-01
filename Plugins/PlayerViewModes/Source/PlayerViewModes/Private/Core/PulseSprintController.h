#pragma once

#include "CoreMinimal.h"

class ACharacter;
class APlayerController;

/**
 * Sprint is a renewable pulse, not a hold.
 * Repeated Shift taps refresh the sprint window; stop tapping and it expires.
 */
class FPulseSprintController
{
public:
    void Enter(ACharacter* Character);
    void Exit();
    void Tick(APlayerController* PC, float DeltaSeconds);

private:
    TWeakObjectPtr<ACharacter> Character;
    float BaseWalkSpeed = 500.0f;
    float RemainingSeconds = 0.0f;
    bool bAppliedSprint = false;

    void ApplySprint(bool bEnable);
};
