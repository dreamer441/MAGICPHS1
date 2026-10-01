#include "PlayerFreeRoamCamera.h"

#include "Camera/CameraComponent.h"

APlayerFreeRoamCamera::APlayerFreeRoamCamera()
{
    PrimaryActorTick.bCanEverTick = false;

    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("FreeRoamCamera"));
    SetRootComponent(Camera);
    Camera->SetFieldOfView(80.0f);
}

void APlayerFreeRoamCamera::SetSpatialView(const FVector& Location, const FRotator& Rotation)
{
    SetActorLocationAndRotation(Location, Rotation);
}

void APlayerFreeRoamCamera::AddSpatialOffset(const FVector& WorldOffset)
{
    AddActorWorldOffset(WorldOffset, false);
}

void APlayerFreeRoamCamera::AddLookDelta(const float YawDelta, const float PitchDelta)
{
    FRotator Rotation = GetActorRotation();
    Rotation.Yaw = FRotator::NormalizeAxis(Rotation.Yaw + YawDelta);
    Rotation.Pitch = FMath::ClampAngle(Rotation.Pitch + PitchDelta, -89.0f, 89.0f);
    Rotation.Roll = 0.0f;
    SetActorRotation(Rotation);
}
