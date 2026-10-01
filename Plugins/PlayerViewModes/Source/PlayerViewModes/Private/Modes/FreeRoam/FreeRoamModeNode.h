#pragma once

#include "CoreMinimal.h"
#include "Core/ControllerInputLock.h"
#include "Core/PlayerGameplayModeNode.h"
#include "FreeRoamModeNode.generated.h"

class ACharacter;
class APlayerFreeRoamCamera;

UCLASS(NotBlueprintable)
class UFreeRoamModeNode final : public UPlayerGameplayModeNode
{
    GENERATED_BODY()

public:
    virtual EPlayerGameplayMode GetMode() const override { return EPlayerGameplayMode::FreeRoam; }
    virtual FPlayerModeCapabilities GetCapabilities() const override;

    virtual void Enter(APlayerController* PC) override;
    virtual void Exit(APlayerController* PC) override;
    virtual void TickMode(APlayerController* PC, float DeltaSeconds) override;
    virtual void ResumeAfterExternalView(APlayerController* PC) override;
    virtual bool GetCastRay(FVector& OutOrigin, FVector& OutDirection) const override;
    virtual bool GetInteractionPoint(FVector& OutPoint) const override;

private:
    TWeakObjectPtr<APlayerFreeRoamCamera> Camera;
    TWeakObjectPtr<ACharacter> Body;
    FControllerInputLock InputLock;

    bool bHavePointerPoint = false;
    FVector PointerPoint = FVector::ZeroVector;

    bool bHaveInteractionPoint = false;
    FVector InteractionPoint = FVector::ZeroVector;

    bool bHaveBodyDestination = false;
    FVector BodyDestination = FVector::ZeroVector;

    int32 PreviousJumpMaxCount = 1;

    // RMB dual action:
    // quick click/release = command body to walk there
    // hold nearly still for 3 seconds = teleport body there
    float RightHoldSeconds = 0.0f;
    bool bRightHoldTriggeredTeleport = false;
    bool bRightHoldActive = false;
    FVector RightHoldAnchorPoint = FVector::ZeroVector;

    APlayerFreeRoamCamera* GetOrSpawnCamera(APlayerController* PC);
    void ApplyPresentation(APlayerController* PC);

    void HandleCameraInput(APlayerController* PC, float DeltaSeconds);
    void UpdatePointerTarget(APlayerController* PC);
    void DrawPointerIndicator() const;

    void HandleRightMouseAction(APlayerController* PC, float DeltaSeconds);
    void UpdateBodyMovement();

    void TeleportBodyTo(const FVector& WorldPoint);
    bool FindWorldPoint(APlayerController* PC, FVector& OutPoint) const;
};
