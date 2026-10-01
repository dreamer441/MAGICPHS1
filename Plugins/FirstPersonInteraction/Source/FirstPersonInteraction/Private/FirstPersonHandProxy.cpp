#include "FirstPersonHandProxy.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

AFirstPersonHandProxy::AFirstPersonHandProxy()
{
    PrimaryActorTick.bCanEverTick = false;

    HandRoot =
        CreateDefaultSubobject<USceneComponent>(
            TEXT("HandRoot"));
    SetRootComponent(HandRoot);

    HandMesh =
        CreateDefaultSubobject<UStaticMeshComponent>(
            TEXT("HandMesh"));
    HandMesh->SetupAttachment(HandRoot);
    HandMesh->SetCollisionEnabled(
        ECollisionEnabled::NoCollision);
    HandMesh->SetGenerateOverlapEvents(false);
    HandMesh->SetCastShadow(false);

    // Asset-free prototype representation. The actor/GripPoint contract is the
    // permanent part; this cube can later be replaced by authored hands/arms.
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(
        TEXT("/Engine/BasicShapes/Cube.Cube"));

    if (CubeMesh.Succeeded())
    {
        HandMesh->SetStaticMesh(CubeMesh.Object);
    }

    // Basic engine cube is 100 cm. This becomes a compact hand/gauntlet proxy.
    HandMesh->SetRelativeScale3D(
        FVector(0.20f, 0.105f, 0.065f));

    GripPoint =
        CreateDefaultSubobject<USceneComponent>(
            TEXT("GripPoint"));
    GripPoint->SetupAttachment(HandRoot);
    GripPoint->SetRelativeLocation(
        FVector(12.0f, 0.0f, 0.0f));
}

void AFirstPersonHandProxy::InitializeHand(
    const EFirstPersonHandSide InSide)
{
    Side = InSide;

    // Mirror a small roll so the two simple proxies read as separate hands.
    HandMesh->SetRelativeRotation(
        FRotator(
            0.0f,
            0.0f,
            Side == EFirstPersonHandSide::Left
                ? -8.0f
                : 8.0f));
}

void AFirstPersonHandProxy::SetHandTransform(
    const FVector& WorldLocation,
    const FRotator& WorldRotation,
    const float DeltaSeconds)
{
    if (bHavePreviousLocation &&
        DeltaSeconds > KINDA_SMALL_NUMBER)
    {
        LinearVelocityCmS =
            (WorldLocation - PreviousLocation) /
            DeltaSeconds;
    }
    else
    {
        LinearVelocityCmS =
            FVector::ZeroVector;
    }

    PreviousLocation = WorldLocation;
    bHavePreviousLocation = true;

    SetActorLocationAndRotation(
        WorldLocation,
        WorldRotation,
        false,
        nullptr,
        ETeleportType::None);
}
