#include "EarthBlockActor.h"
#include "EarthGeometryComponent.h"
#include "EarthMaterialComponent.h"
#include "EarthSubtractionComponent.h"
#include "ImpactSolver.h"
#include "Components/DynamicMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "PhysicalMaterials/PhysicalMaterial.h"

AEarthBlockActor::AEarthBlockActor()
{
    PrimaryActorTick.bCanEverTick = false;
    EarthMesh = CreateDefaultSubobject<UDynamicMeshComponent>(TEXT("EarthMesh"));
    SetRootComponent(EarthMesh);
    MaterialState = CreateDefaultSubobject<UEarthMaterialComponent>(TEXT("MaterialState"));
    Geometry = CreateDefaultSubobject<UEarthGeometryComponent>(TEXT("Geometry"));
    Subtraction = CreateDefaultSubobject<UEarthSubtractionComponent>(TEXT("Subtraction"));
    EarthMesh->SetMobility(EComponentMobility::Movable);
    EarthMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    EarthMesh->SetCollisionResponseToAllChannels(ECR_Block);
    EarthMesh->SetNotifyRigidBodyCollision(true);
}
void AEarthBlockActor::OnConstruction(const FTransform& Transform) { Super::OnConstruction(Transform); RebuildEarthBlock(); }
void AEarthBlockActor::BeginPlay() { Super::BeginPlay(); RefreshPhysicalMaterial(); }
void AEarthBlockActor::RebuildEarthBlock()
{
    if (Geometry && EarthMesh)
    {
        Geometry->RebuildBlock(EarthMesh);
        if (VisualMaterial) EarthMesh->SetMaterial(0, VisualMaterial);
        RefreshPhysicalMaterial();
    }
}
void AEarthBlockActor::SetEarthVisualMaterial(UMaterialInterface* NewMaterial)
{
    VisualMaterial = NewMaterial;
    if (EarthMesh && VisualMaterial) EarthMesh->SetMaterial(0, VisualMaterial);
}
void AEarthBlockActor::RefreshPhysicalMaterial()
{
    if (!EarthMesh || !MaterialState) return;
    if (!RuntimePhysicalMaterial) RuntimePhysicalMaterial = NewObject<UPhysicalMaterial>(this, TEXT("RuntimeEarthPhysicalMaterial"));
    const FMaterialPhysicalProperties P = MaterialState->GetMaterialState();
    RuntimePhysicalMaterial->Friction = FMath::Max(0.0f, P.Friction);
    RuntimePhysicalMaterial->Restitution = FMath::Clamp(P.Restitution, 0.0f, 1.0f);
    RuntimePhysicalMaterial->Density = FMath::Max(0.001f, P.DensityKgPerM3 / 1000.0f);
    RuntimePhysicalMaterial->FrictionCombineMode = EFrictionCombineMode::Average;
    RuntimePhysicalMaterial->bOverrideFrictionCombineMode = true;
    EarthMesh->SetPhysMaterialOverride(RuntimePhysicalMaterial);
}
bool AEarthBlockActor::ReceiveImpact_Implementation(const FImpactRequest& Request, FImpactResult& OutResult)
{
    if (!MaterialState || !Subtraction || !EarthMesh) { OutResult = FImpactResult(); return false; }
    FImpactRequest CanonicalRequest = Request;
    FVector SurfaceNormal = CanonicalRequest.WorldSurfaceNormal.GetSafeNormal();
    const FVector VelocityDirection = CanonicalRequest.Source.VelocityCmS.GetSafeNormal();
    if (SurfaceNormal.IsNearlyZero()) SurfaceNormal = -VelocityDirection;
    if (!VelocityDirection.IsNearlyZero() && FVector::DotProduct(VelocityDirection, SurfaceNormal) > 0.0f) SurfaceNormal *= -1.0f;
    CanonicalRequest.WorldSurfaceNormal = SurfaceNormal;
    LastImpactResult = UImpactSolver::SolveSubtractiveImpact(CanonicalRequest, MaterialState->GetMaterialState(), ImpactTuning);
    OutResult = LastImpactResult;
    if (!LastImpactResult.Target.bProducesRemoval) return false;
    return Subtraction->ApplySolvedSubtraction(EarthMesh, CanonicalRequest, LastImpactResult);
}
