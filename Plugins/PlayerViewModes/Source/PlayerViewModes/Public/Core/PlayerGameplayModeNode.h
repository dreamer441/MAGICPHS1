#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Core/PlayerGameplayModeTypes.h"
#include "PlayerGameplayModeNode.generated.h"

class APlayerController;

/**
 * Small gameplay-mode node contract.
 *
 * Nodes never transition to one another. The subsystem owns transitions.
 * Each node owns only the camera/input/movement behavior for its own mode.
 */
UCLASS(Abstract, NotBlueprintable)
class PLAYERVIEWMODES_API UPlayerGameplayModeNode : public UObject
{
    GENERATED_BODY()

public:
    virtual EPlayerGameplayMode GetMode() const { return EPlayerGameplayMode::ThirdPerson; }
    virtual FPlayerModeCapabilities GetCapabilities() const { return FPlayerModeCapabilities(); }

    virtual void Enter(APlayerController* PC) {}
    virtual void Exit(APlayerController* PC) {}
    virtual void TickMode(APlayerController* PC, float DeltaSeconds) {}
    virtual void ResumeAfterExternalView(APlayerController* PC) {}

    /** Optional aim contract. Currently implemented by TopDown. */
    virtual bool GetCastRay(FVector& OutOrigin, FVector& OutDirection) const { return false; }

    /** Optional world-point contract. Currently implemented by FreeRoam. */
    virtual bool GetInteractionPoint(FVector& OutPoint) const { return false; }
};
