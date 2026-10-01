#pragma once

#include "CoreMinimal.h"
#include "Core/ControllerInputLock.h"
#include "Core/PlayerGameplayModeNode.h"
#include "TopDownModeNode.generated.h"

class ACharacter;
class APlayerTopDownCamera;

UCLASS(NotBlueprintable)
class UTopDownModeNode final : public UPlayerGameplayModeNode
{
    GENERATED_BODY()

public:
    virtual EPlayerGameplayMode GetMode() const override { return EPlayerGameplayMode::TopDown; }
    virtual FPlayerModeCapabilities GetCapabilities() const override;

    virtual void Enter(APlayerController* PC) override;
    virtual void Exit(APlayerController* PC) override;
    virtual void TickMode(APlayerController* PC, float DeltaSeconds) override;
    virtual void ResumeAfterExternalView(APlayerController* PC) override;
    virtual bool GetCastRay(FVector& OutOrigin, FVector& OutDirection) const override;

private:
    TWeakObjectPtr<APlayerTopDownCamera> Camera;
    TWeakObjectPtr<ACharacter> Character;
    FControllerInputLock InputLock;

    bool bSprinting = false;
    float SprintWindowRemainingSeconds = 0.0f;
    bool bHaveDestination = false;
    FVector MoveDestination = FVector::ZeroVector;

    float OrbitYawDegrees = 0.0f;
    float OrbitElevationDegrees = 62.0f;

    float PreviousWalkSpeed = 500.0f;
    bool bPreviousOrientRotationToMovement = false;
    bool bPreviousUseControllerRotationYaw = false;
    int32 PreviousJumpMaxCount = 1;

    APlayerTopDownCamera* GetOrSpawnCamera(APlayerController* PC);
    void ApplyPresentation(APlayerController* PC);
    void UpdateCamera(APlayerController* PC);
    void HandleInput(APlayerController* PC, float DeltaSeconds);
    void SetSprint(bool bEnabled);
    bool FindCursorWorldPoint(APlayerController* PC, FVector& OutPoint) const;
};
