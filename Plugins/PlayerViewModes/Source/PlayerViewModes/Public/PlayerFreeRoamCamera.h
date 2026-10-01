#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PlayerFreeRoamCamera.generated.h"

class UCameraComponent;

/** Detached spatial camera used by FreeRoam mode. */
UCLASS(NotBlueprintable)
class PLAYERVIEWMODES_API APlayerFreeRoamCamera : public AActor
{
    GENERATED_BODY()

public:
    APlayerFreeRoamCamera();

    void SetSpatialView(const FVector& Location, const FRotator& Rotation);
    void AddSpatialOffset(const FVector& WorldOffset);
    void AddLookDelta(float YawDelta, float PitchDelta);

private:
    UPROPERTY(VisibleAnywhere, Category="Camera")
    TObjectPtr<UCameraComponent> Camera;
};
