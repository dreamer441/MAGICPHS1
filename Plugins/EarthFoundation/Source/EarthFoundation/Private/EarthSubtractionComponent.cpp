#include "EarthSubtractionComponent.h"

#include "Components/DynamicMeshComponent.h"
#include "DynamicMesh/DynamicMesh3.h"
#include "Generators/SphereGenerator.h"
#include "Operations/MeshBoolean.h"
#include "UDynamicMesh.h"

using namespace UE::Geometry;

UEarthSubtractionComponent::UEarthSubtractionComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

bool UEarthSubtractionComponent::ApplySolvedSubtraction(
    UDynamicMeshComponent* TargetMesh,
    const FImpactRequest& Request,
    const FImpactResult& Result)
{
    if (!TargetMesh || !Result.Target.bProducesRemoval || Result.Target.CraterRadiusCm <= 0.0f || Result.Target.PenetrationDepthCm <= 0.0f)
    {
        return false;
    }

    const FTransform MeshTransform = TargetMesh->GetComponentTransform();
    const FVector LocalImpact = MeshTransform.InverseTransformPosition(Request.WorldPosition);

    FVector LocalOutwardNormal = MeshTransform.InverseTransformVectorNoScale(Request.WorldSurfaceNormal).GetSafeNormal();
    if (LocalOutwardNormal.IsNearlyZero())
    {
        LocalOutwardNormal = FVector::UpVector;
    }

    FVector WorldInwardDirection = Result.Target.WorldPenetrationDirection.GetSafeNormal();
    if (WorldInwardDirection.IsNearlyZero())
    {
        WorldInwardDirection = -Request.WorldSurfaceNormal.GetSafeNormal();
    }

    FVector LocalInwardDirection = MeshTransform.InverseTransformVectorNoScale(WorldInwardDirection).GetSafeNormal();
    if (LocalInwardDirection.IsNearlyZero())
    {
        LocalInwardDirection = -LocalOutwardNormal;
    }
    if (FVector::DotProduct(LocalInwardDirection, LocalOutwardNormal) > 0.0f)
    {
        LocalInwardDirection *= -1.0f;
    }

    // Build a stable orthonormal basis whose X axis points along penetration.
    const FVector ReferenceAxis = FMath::Abs(LocalInwardDirection.Z) < 0.95f ? FVector::UpVector : FVector::RightVector;
    FVector LocalSideAxis = FVector::CrossProduct(ReferenceAxis, LocalInwardDirection).GetSafeNormal();
    if (LocalSideAxis.IsNearlyZero())
    {
        LocalSideAxis = FVector::RightVector;
    }
    const FVector LocalUpAxis = FVector::CrossProduct(LocalInwardDirection, LocalSideAxis).GetSafeNormal();

    // Keep the ellipsoid centered on the contact surface. Roughly one half is therefore inside
    // a locally flat target, which matches the half-ellipsoid volume used by ImpactSystem.
    const FVector LocalCutterCenter = LocalImpact - (LocalOutwardNormal * SurfaceInsetCm);

    FSphereGenerator CutterGenerator;
    CutterGenerator.Radius = 1.0;
    CutterGenerator.NumPhi = FMath::Clamp(CutterResolution, 8, 48);
    CutterGenerator.NumTheta = FMath::Clamp(CutterResolution, 8, 48);
    CutterGenerator.bPolygroupPerQuad = false;
    CutterGenerator.Generate();

    FDynamicMesh3 CutterMesh(&CutterGenerator);
    const double DepthCm = static_cast<double>(Result.Target.PenetrationDepthCm);
    const double RadiusCm = static_cast<double>(Result.Target.CraterRadiusCm);
    const FVector3d Center(LocalCutterCenter);
    const FVector3d AxisDepth(LocalInwardDirection);
    const FVector3d AxisSide(LocalSideAxis);
    const FVector3d AxisUp(LocalUpAxis);

    for (int32 VertexID : CutterMesh.VertexIndicesItr())
    {
        const FVector3d UnitPoint = CutterMesh.GetVertex(VertexID);
        const FVector3d EllipsoidPoint = Center
            + AxisDepth * (UnitPoint.X * DepthCm)
            + AxisSide * (UnitPoint.Y * RadiusCm)
            + AxisUp * (UnitPoint.Z * RadiusCm);
        CutterMesh.SetVertex(VertexID, EllipsoidPoint);
    }

    bool bBooleanSucceeded = false;

    TargetMesh->EditMesh(
        [&](FDynamicMesh3& Mesh)
        {
            FDynamicMesh3 InputMesh(Mesh);
            FDynamicMesh3 BooleanResult;

            FMeshBoolean Difference(
                &InputMesh,
                &CutterMesh,
                &BooleanResult,
                FMeshBoolean::EBooleanOp::Difference);

            Difference.bSimplifyAlongNewEdges = true;
            Difference.bCollapseDegenerateEdgesOnCut = true;

            if (Difference.Compute())
            {
                Mesh = MoveTemp(BooleanResult);
                bBooleanSucceeded = true;
            }
        },
        EDynamicMeshComponentRenderUpdateMode::FullUpdate,
        EDynamicMeshChangeType::GeneralEdit,
        EDynamicMeshAttributeChangeFlags::Unknown);

    if (bBooleanSucceeded)
    {
        TargetMesh->UpdateCollision(false);
    }

    return bBooleanSucceeded;
}
