#include "Core/PulseSprintController.h"

#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"

namespace PulseSprint
{
    constexpr float PulseSeconds = 0.60f;
    constexpr float SpeedMultiplier = 1.75f;
}

void FPulseSprintController::Enter(ACharacter* InCharacter)
{
    Exit();
    Character = InCharacter;

    if (ACharacter* Current = Character.Get())
    {
        if (UCharacterMovementComponent* Movement = Current->GetCharacterMovement())
        {
            BaseWalkSpeed = Movement->MaxWalkSpeed;
        }
    }
}

void FPulseSprintController::Exit()
{
    ApplySprint(false);
    Character.Reset();
    RemainingSeconds = 0.0f;
}

void FPulseSprintController::ApplySprint(const bool bEnable)
{
    if (bAppliedSprint == bEnable)
    {
        return;
    }

    bAppliedSprint = bEnable;

    if (ACharacter* Current = Character.Get())
    {
        if (UCharacterMovementComponent* Movement = Current->GetCharacterMovement())
        {
            Movement->MaxWalkSpeed = BaseWalkSpeed * (bEnable ? PulseSprint::SpeedMultiplier : 1.0f);
        }
    }
}

void FPulseSprintController::Tick(APlayerController* PC, const float DeltaSeconds)
{
    if (!PC || !Character.IsValid())
    {
        return;
    }

    if (PC->WasInputKeyJustPressed(EKeys::LeftShift) ||
        PC->WasInputKeyJustPressed(EKeys::RightShift))
    {
        RemainingSeconds = PulseSprint::PulseSeconds;
    }
    else
    {
        RemainingSeconds = FMath::Max(0.0f, RemainingSeconds - DeltaSeconds);
    }

    const bool bMovementKeyDown =
        PC->IsInputKeyDown(EKeys::W) ||
        PC->IsInputKeyDown(EKeys::A) ||
        PC->IsInputKeyDown(EKeys::S) ||
        PC->IsInputKeyDown(EKeys::D);

    ApplySprint(bMovementKeyDown && RemainingSeconds > 0.0f);
}
