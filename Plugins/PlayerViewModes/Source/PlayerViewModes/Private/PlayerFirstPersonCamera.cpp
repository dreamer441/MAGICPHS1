#include "PlayerFirstPersonCamera.h"

#include "Camera/CameraComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"

APlayerFirstPersonCamera::APlayerFirstPersonCamera()
{
    PrimaryActorTick.bCanEverTick = false;

    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
    SetRootComponent(Camera);
    Camera->SetFieldOfView(90.0f);
}

void APlayerFirstPersonCamera::Follow(ACharacter* Character, APlayerController* PC)
{
    if (!Character || !PC)
    {
        return;
    }

    const FRotator ViewRotation = PC->GetControlRotation();

    // Small forward offset keeps the prototype camera out of the template head mesh.
    const FVector EyePosition =
        Character->GetPawnViewLocation() + ViewRotation.Vector() * 18.0f;

    SetActorLocationAndRotation(EyePosition, ViewRotation);
}
