#include "PlayerTopDownCamera.h"

#include "Camera/CameraComponent.h"
#include "Components/SceneComponent.h"
#include "GameFramework/SpringArmComponent.h"

APlayerTopDownCamera::APlayerTopDownCamera()
{
    PrimaryActorTick.bCanEverTick = false;

    Pivot = CreateDefaultSubobject<USceneComponent>(TEXT("FollowPivot"));
    SetRootComponent(Pivot);

    CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("OrbitBoom"));
    CameraBoom->SetupAttachment(Pivot);
    CameraBoom->TargetArmLength = 2200.0f;
    CameraBoom->bUsePawnControlRotation = false;
    CameraBoom->bInheritPitch = false;
    CameraBoom->bInheritYaw = false;
    CameraBoom->bInheritRoll = false;
    CameraBoom->bDoCollisionTest = true;
    CameraBoom->ProbeSize = 14.0f;
    CameraBoom->bEnableCameraLag = true;
    CameraBoom->CameraLagSpeed = 9.0f;
    CameraBoom->bEnableCameraRotationLag = true;
    CameraBoom->CameraRotationLagSpeed = 14.0f;
    CameraBoom->bUseCameraLagSubstepping = true;

    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("TopDownCamera"));
    Camera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
    Camera->SetFieldOfView(65.0f);

    SetOrbit(0.0f, 62.0f);
}

void APlayerTopDownCamera::SetOrbit(const float OrbitYaw, const float ElevationDegrees)
{
    CameraBoom->SetRelativeRotation(FRotator(-ElevationDegrees, OrbitYaw, 0.0f));
}

void APlayerTopDownCamera::FollowPosition(const FVector& WorldLocation)
{
    SetActorLocation(WorldLocation + FVector(0.0f, 0.0f, 130.0f));
}
