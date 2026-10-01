#include "EarthGeometryComponent.h"

#include "Components/DynamicMeshComponent.h"
#include "DynamicMesh/DynamicMesh3.h"
#include "Generators/GridBoxMeshGenerator.h"
#include "OrientedBoxTypes.h"

using namespace UE::Geometry;

UEarthGeometryComponent::UEarthGeometryComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UEarthGeometryComponent::RebuildBlock(UDynamicMeshComponent* TargetMesh)
{
    if (!TargetMesh)
    {
        return;
    }

    ResolvedBlockSizeCm = FVector(
        FMath::Max(100.0, BlockSizeCm.X),
        FMath::Max(100.0, BlockSizeCm.Y),
        FMath::Max(20.0, BlockSizeCm.Z));

    const int32 XYResolution = FMath::Clamp(SurfaceResolution, 5, 101);
    const int32 ZResolution = FMath::Clamp(VerticalResolution, 2, 101);

    FGridBoxMeshGenerator Generator;
    Generator.Box = FOrientedBox3d(
        FVector3d::Zero(),
        FVector3d(ResolvedBlockSizeCm.X * 0.5, ResolvedBlockSizeCm.Y * 0.5, ResolvedBlockSizeCm.Z * 0.5));
    Generator.EdgeVertices = FIndex3i(XYResolution, XYResolution, ZResolution);
    Generator.bPolygroupPerQuad = false;
    Generator.Generate();

    FDynamicMesh3 Mesh(&Generator);

    TargetMesh->SetMesh(MoveTemp(Mesh));
    TargetMesh->SetComplexAsSimpleCollisionEnabled(true, true);
    TargetMesh->SetDeferredCollisionUpdatesEnabled(false, true);
    TargetMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    TargetMesh->SetCollisionResponseToAllChannels(ECR_Block);
    TargetMesh->SetNotifyRigidBodyCollision(true);
}

