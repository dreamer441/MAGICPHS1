#pragma once

#include "CoreMinimal.h"
#include "Core/Phase1GameplayModeTypes.h"

class APhase1Character;
class APlayerController;

/**
 * Plain C++ gameplay-state contract.
 *
 * The four modes do not know about one another. The world subsystem owns
 * transitions and delegates Enter/Exit/Tick to one active node.
 */
class PHASE1GAMEPLAY_API FPhase1ModeNode
{
public:
    virtual ~FPhase1ModeNode() = default;

    virtual EPhase1GameplayMode GetMode() const = 0;
    virtual FPhase1ModeCapabilities GetCapabilities() const = 0;

    virtual void Enter(APlayerController* PC) = 0;
    virtual void Exit(APlayerController* PC) = 0;
    virtual void Tick(APlayerController* PC, float DeltaSeconds) = 0;

    virtual bool GetCastRay(APlayerController* PC, FVector& OutOrigin, FVector& OutDirection) const;
    virtual bool GetInteractionPoint(FVector& OutPoint) const;

protected:
    APhase1Character* GetCharacter(APlayerController* PC) const;

    void ApplyMouseLook(APlayerController* PC, float DegreesPerMouseUnit = 0.18f) const;
    void ApplyCameraRelativeGroundMove(APlayerController* PC) const;
    void ApplyJumpInput(APlayerController* PC) const;

    bool CursorPointOnHorizontalPlane(
        APlayerController* PC,
        float PlaneZ,
        FVector& OutPoint) const;

    bool CameraCenterRay(
        APlayerController* PC,
        FVector& OutOrigin,
        FVector& OutDirection) const;

    void SetWorldCursorMode(APlayerController* PC, bool bEnabled) const;
    bool IsShiftJustPressed(APlayerController* PC) const;
};
