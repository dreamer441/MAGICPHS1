#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PlayerFirstPersonCamera.generated.h"

class ACharacter;
class APlayerController;
class UCameraComponent;

/** Presentation-only first-person camera. */
UCLASS(NotBlueprintable)
class PLAYERVIEWMODES_API APlayerFirstPersonCamera : public AActor
{
    GENERATED_BODY()

public:
    APlayerFirstPersonCamera();

    void Follow(ACharacter* Character, APlayerController* PC);

private:
    UPROPERTY(VisibleAnywhere, Category="Camera")
    TObjectPtr<UCameraComponent> Camera;
};
