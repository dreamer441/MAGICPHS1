#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PlayerTopDownCamera.generated.h"

class UCameraComponent;
class USceneComponent;
class USpringArmComponent;

/** Presentation node only: spring-arm orbit camera; knows nothing about input or spells. */
UCLASS(NotBlueprintable)
class PLAYERVIEWMODES_API APlayerTopDownCamera : public AActor
{
    GENERATED_BODY()
public:
    APlayerTopDownCamera();

    /** OrbitYaw is world degrees; Elevation is positive angle ABOVE horizontal. */
    void SetOrbit(float OrbitYaw, float ElevationDegrees);
    void FollowPosition(const FVector& WorldLocation);

private:
    UPROPERTY(VisibleAnywhere, Category="Camera")
    TObjectPtr<USceneComponent> Pivot;

    UPROPERTY(VisibleAnywhere, Category="Camera")
    TObjectPtr<USpringArmComponent> CameraBoom;

    UPROPERTY(VisibleAnywhere, Category="Camera")
    TObjectPtr<UCameraComponent> Camera;
};
