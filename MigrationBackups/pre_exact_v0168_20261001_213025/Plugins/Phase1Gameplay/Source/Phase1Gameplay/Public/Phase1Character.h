#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Phase1Character.generated.h"

class UCameraComponent;
class USpringArmComponent;

/**
 * Deliberately presentation-free test character.
 *
 * No skeletal mesh, animation blueprint, marketplace content or landscape
 * dependency is required. Presentation can be layered on later without changing
 * the gameplay-mode contract.
 */
UCLASS()
class PHASE1GAMEPLAY_API APhase1Character : public ACharacter
{
    GENERATED_BODY()

public:
    APhase1Character();

    void ConfigureFirstPersonCamera();
    void ConfigureThirdPersonCamera(bool bPrecisionAim);
    void ConfigureTopDownCamera(float OrbitYawDegrees);

    void RestoreGroundMovement();
    void SetPhase1WalkSpeed(float NewSpeedCmPerSecond);

    UCameraComponent* GetPhase1Camera() const { return Phase1Camera; }
    USpringArmComponent* GetPhase1SpringArm() const { return CameraBoom; }

private:
    UPROPERTY(VisibleAnywhere, Category = "Phase 1|Camera")
    TObjectPtr<USpringArmComponent> CameraBoom;

    UPROPERTY(VisibleAnywhere, Category = "Phase 1|Camera")
    TObjectPtr<UCameraComponent> Phase1Camera;
};
