#include "Phase1Character.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"

APhase1Character::APhase1Character()
{
    PrimaryActorTick.bCanEverTick = false;

    GetCapsuleComponent()->InitCapsuleSize(42.0f, 96.0f);

    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw = true;
    bUseControllerRotationRoll = false;

    UCharacterMovementComponent* Movement = GetCharacterMovement();
    Movement->bOrientRotationToMovement = false;
    Movement->RotationRate = FRotator(0.0f, 540.0f, 0.0f);
    Movement->JumpZVelocity = 620.0f;
    Movement->AirControl = 0.35f;
    Movement->MaxWalkSpeed = 500.0f;
    Movement->BrakingDecelerationWalking = 1600.0f;

    // ACharacter owns a skeletal-mesh component, but this clean foundation
    // deliberately does not assign a mesh or animation asset.
    GetMesh()->SetVisibility(false, true);
    GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("Phase1CameraBoom"));
    CameraBoom->SetupAttachment(GetRootComponent());
    CameraBoom->TargetArmLength = 360.0f;
    CameraBoom->bUsePawnControlRotation = true;
    CameraBoom->bDoCollisionTest = true;
    CameraBoom->ProbeSize = 12.0f;
    CameraBoom->SocketOffset = FVector(0.0f, 25.0f, 65.0f);

    Phase1Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Phase1Camera"));
    Phase1Camera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
    Phase1Camera->bUsePawnControlRotation = false;
}

void APhase1Character::ConfigureFirstPersonCamera()
{
    bUseControllerRotationYaw = true;

    CameraBoom->bUsePawnControlRotation = true;
    CameraBoom->bDoCollisionTest = false;
    CameraBoom->TargetArmLength = 0.0f;
    CameraBoom->SocketOffset = FVector(0.0f, 0.0f, 72.0f);
    CameraBoom->SetRelativeRotation(FRotator::ZeroRotator);
}

void APhase1Character::ConfigureThirdPersonCamera(const bool bPrecisionAim)
{
    bUseControllerRotationYaw = true;

    CameraBoom->bUsePawnControlRotation = true;
    CameraBoom->bDoCollisionTest = true;
    CameraBoom->SetRelativeRotation(FRotator::ZeroRotator);

    if (bPrecisionAim)
    {
        CameraBoom->TargetArmLength = 165.0f;
        CameraBoom->SocketOffset = FVector(0.0f, 62.0f, 66.0f);
    }
    else
    {
        CameraBoom->TargetArmLength = 360.0f;
        CameraBoom->SocketOffset = FVector(0.0f, 25.0f, 65.0f);
    }
}

void APhase1Character::ConfigureTopDownCamera(const float OrbitYawDegrees)
{
    bUseControllerRotationYaw = false;

    CameraBoom->bUsePawnControlRotation = false;
    CameraBoom->bDoCollisionTest = false;
    CameraBoom->TargetArmLength = 900.0f;
    CameraBoom->SocketOffset = FVector(0.0f, 0.0f, 40.0f);
    CameraBoom->SetRelativeRotation(FRotator(-62.0f, OrbitYawDegrees, 0.0f));
}

void APhase1Character::RestoreGroundMovement()
{
    if (UCharacterMovementComponent* Movement = GetCharacterMovement())
    {
        Movement->SetMovementMode(MOVE_Walking);
        Movement->StopMovementImmediately();
    }
}

void APhase1Character::SetPhase1WalkSpeed(const float NewSpeedCmPerSecond)
{
    if (UCharacterMovementComponent* Movement = GetCharacterMovement())
    {
        Movement->MaxWalkSpeed = FMath::Max(1.0f, NewSpeedCmPerSecond);
    }
}
