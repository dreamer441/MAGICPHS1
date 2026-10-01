#include "EarthSpellBody.h"

#include "EarthSpellShapeBuilder.h"
#include "EarthSpellDamageGeometry.h"
#include "SpellShapeMath.h"
#include "EarthMaterialComponent.h"
#include "EarthSubtractionComponent.h"
#include "ImpactSolver.h"
#include "Components/StaticMeshComponent.h"
#include "Components/DynamicMeshComponent.h"
#include "DynamicMesh/DynamicMesh3.h"
#include "Engine/Engine.h"
#include "Materials/MaterialInterface.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "UObject/ConstructorHelpers.h"

AEarthSpellBody::AEarthSpellBody()
{
    PrimaryActorTick.bCanEverTick = true;
    // Earth persists. Lifetime is never a hidden projectile timer.
    InitialLifeSpan = 0.0f;

    BodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PhysicalBodyProxy"));
    SetRootComponent(BodyMesh);
    BodyMesh->SetMobility(EComponentMobility::Movable);
    BodyMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    BodyMesh->SetCollisionObjectType(ECC_WorldDynamic);
    BodyMesh->SetCollisionResponseToAllChannels(ECR_Block);
    // v0.8 incorrectly ignored the player. Earth must be physically solid.
    BodyMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
    BodyMesh->SetNotifyRigidBodyCollision(true);
    BodyMesh->SetSimulatePhysics(false);
    BodyMesh->SetEnableGravity(true);
    BodyMesh->SetLinearDamping(0.02f);
    BodyMesh->SetAngularDamping(0.04f);
    BodyMesh->BodyInstance.bUseCCD = true;

    EditableMesh = CreateDefaultSubobject<UDynamicMeshComponent>(TEXT("EditableEarthGeometry"));
    EditableMesh->SetupAttachment(BodyMesh);
    EditableMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    EditableMesh->SetVisibility(false);

    MaterialState = CreateDefaultSubobject<UEarthMaterialComponent>(TEXT("EarthMaterialState"));
    Subtraction = CreateDefaultSubobject<UEarthSubtractionComponent>(TEXT("EarthSubtraction"));

    static ConstructorHelpers::FObjectFinder<UMaterialInterface> ProjectEarthMaterial(
        TEXT("/Game/AMADEUS/Materials/M_AMADEUS_Earth_v13.M_AMADEUS_Earth_v13"));
    if (ProjectEarthMaterial.Succeeded())
    {
        EarthVisualMaterial = ProjectEarthMaterial.Object;
    }
    else
    {
        static ConstructorHelpers::FObjectFinder<UMaterialInterface> BasicShapeMaterial(
            TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
        if (BasicShapeMaterial.Succeeded())
        {
            EarthVisualMaterial = BasicShapeMaterial.Object;
        }
    }

    BodyMesh->OnComponentHit.AddDynamic(this, &AEarthSpellBody::HandleBodyHit);
}

void AEarthSpellBody::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    ImpactCooldownRemaining = FMath::Max(ImpactCooldownRemaining - DeltaSeconds, 0.0f);

    if (BodyMesh && BodyMesh->IsSimulatingPhysics() && ImpactCooldownRemaining <= 0.0f)
    {
        const FVector CurrentVelocity = BodyMesh->GetPhysicsLinearVelocity();
        if (!CurrentVelocity.IsNearlyZero())
        {
            LastFreeVelocityCmS = CurrentVelocity;
        }
    }
    TryActivateExactSettledCollision(DeltaSeconds);
}

void AEarthSpellBody::Configure(const FEarthSpellDefinition& InSpell)
{
    ConfigureResolvedSpell(
        FSpellDefinitionAdapter::Resolve(FSpellDefinitionAdapter::FromLegacyEarth(InSpell)));
}

void AEarthSpellBody::ConfigureResolvedSpell(const FResolvedSpell& InSpell)
{
    // Re-resolve definitions at the Earth boundary so geometry and body data stay
    // coherent even if an external caller supplies a stale derived body snapshot.
    RuntimeSpell = FSpellDefinitionAdapter::Resolve(InSpell.Definition);
    Spell = FSpellDefinitionAdapter::ToLegacyEarth(RuntimeSpell.Definition);
    Spell.MassKg = RuntimeSpell.Body.MassKg;
    SolidVolumeM3 = FMath::Max(RuntimeSpell.Body.VolumeM3, 0.000001f);

    RebuildBodyGeometry();
    RefreshMaterialState();
    RefreshPhysicalMaterial();
}

void AEarthSpellBody::RefreshMaterialState()
{
    if (!MaterialState) return;

    // Preserve every generic material field for impacts and physical response.
    FMaterialPhysicalProperties& Earth = MaterialState->InlineProperties.Material;
    Earth = RuntimeSpell.Body.Material;
    Earth.DensityKgPerM3 = GetBodyDensityKgPerM3();
}

void AEarthSpellBody::SyncEditableMeshScale()
{
    if (!EditableMesh || !BodyMesh) return;
    // Child of the scaled physics proxy, but its vertices are in real cm.
    // Inverse-scale it so the editable geometry and original shape coincide.
    const FVector S = BodyMesh->GetRelativeScale3D();
    EditableMesh->SetRelativeScale3D(FVector(
        1.0f / FMath::Max(FMath::Abs(S.X), 0.0001),
        1.0f / FMath::Max(FMath::Abs(S.Y), 0.0001),
        1.0f / FMath::Max(FMath::Abs(S.Z), 0.0001)));
}

void AEarthSpellBody::RebuildBodyGeometry()
{
    if (!BodyMesh) return;
    const bool bWasSimulating = BodyMesh->IsSimulatingPhysics();
    if (bWasSimulating) BodyMesh->SetSimulatePhysics(false);

    BodyMesh->SetStaticMesh(FEarthSpellShapeBuilder::ResolveStaticMesh(RuntimeSpell.Definition.Shape));
    BodyMesh->SetRelativeScale3D(FEarthSpellShapeBuilder::CalculateMeshScale(RuntimeSpell.Definition));
    BodyMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    BodyMesh->SetCollisionResponseToAllChannels(ECR_Block);
    BodyMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
    BodyMesh->SetNotifyRigidBodyCollision(true);
    BodyMesh->SetMaterial(0, EarthVisualMaterial);
    BodyMesh->SetMassOverrideInKg(NAME_None, GetBodyMassKg(), true);
    SyncEditableMeshScale();

    if (bWasSimulating) BodyMesh->SetSimulatePhysics(true);
}

void AEarthSpellBody::RefreshPhysicalMaterial()
{
    if (!BodyMesh || !MaterialState) return;
    if (!RuntimePhysicalMaterial)
    {
        RuntimePhysicalMaterial = NewObject<UPhysicalMaterial>(this, TEXT("RuntimeEarthSpellPhysicalMaterial"));
    }
    const FMaterialPhysicalProperties Earth = MaterialState->GetMaterialState();
    RuntimePhysicalMaterial->Friction = FMath::Max(Earth.Friction, 0.0f);
    RuntimePhysicalMaterial->Restitution = FMath::Clamp(Earth.Restitution, 0.0f, 1.0f);
    RuntimePhysicalMaterial->Density = FMath::Max(0.001f, Earth.DensityKgPerM3 / 1000.0f);
    RuntimePhysicalMaterial->FrictionCombineMode = EFrictionCombineMode::Average;
    RuntimePhysicalMaterial->bOverrideFrictionCombineMode = true;
    BodyMesh->SetPhysMaterialOverride(RuntimePhysicalMaterial);
    EditableMesh->SetPhysMaterialOverride(RuntimePhysicalMaterial);
}

float AEarthSpellBody::GetBodyVolumeM3() const
{
    return SolidVolumeM3 > 0.0f
        ? SolidVolumeM3
        : FMath::Max(RuntimeSpell.Body.VolumeM3, 0.000001f);
}

float AEarthSpellBody::GetBodyMassKg() const
{
    return FMath::Max(RuntimeSpell.Body.MassKg, 0.01f);
}

float AEarthSpellBody::GetBodyDensityKgPerM3() const
{
    return FMath::Max(GetBodyMassKg() / FMath::Max(GetBodyVolumeM3(), 0.000001f), 1.0f);
}

void AEarthSpellBody::Launch(const FVector& Direction)
{
    RebuildBodyGeometry();
    RefreshPhysicalMaterial();

    ImpactCooldownRemaining = 0.0f;
    const FVector LaunchVelocity = Direction.GetSafeNormal() * (RuntimeSpell.Definition.SpeedMps * 100.0f);
    LastFreeVelocityCmS = LaunchVelocity;

    BodyMesh->SetEnableGravity(RuntimeSpell.Body.bAffectedByGravity);
    BodyMesh->SetSimulatePhysics(true);
    BodyMesh->SetMassOverrideInKg(NAME_None, GetBodyMassKg(), true);
    BodyMesh->SetPhysicsLinearVelocity(LaunchVelocity);
    BodyMesh->WakeAllRigidBodies();
}

FVector AEarthSpellBody::GetIncomingVelocityCmS() const
{
    if (!BodyMesh || !BodyMesh->IsSimulatingPhysics() || !BodyMesh->RigidBodyIsAwake(NAME_None))
    {
        return FVector::ZeroVector;
    }
    return !LastFreeVelocityCmS.IsNearlyZero()
        ? LastFreeVelocityCmS
        : BodyMesh->GetPhysicsLinearVelocity();
}

bool AEarthSpellBody::BuildImpactRequest_Implementation(const FHitResult& Hit, FImpactRequest& OutRequest) const
{
    OutRequest.WorldPosition = Hit.ImpactPoint;
    OutRequest.WorldSurfaceNormal = Hit.ImpactNormal.GetSafeNormal();
    OutRequest.Source.MassKg = GetBodyMassKg();
    OutRequest.Source.VolumeM3 = GetBodyVolumeM3();
    OutRequest.Source.VelocityCmS = GetIncomingVelocityCmS();
    OutRequest.Source.ContactRadiusCm = RuntimeSpell.Body.ContactRadiusCm;
    OutRequest.Source.bAffectedByGravity = RuntimeSpell.Body.bAffectedByGravity;
    OutRequest.Source.Material = MaterialState
        ? MaterialState->GetMaterialState()
        : FMaterialPhysicalProperties();
    return true;
}

void AEarthSpellBody::EnsureEditableMesh()
{
    if (bHasEditableMesh || !EditableMesh) return;
    EditableMesh->SetMesh(FEarthSpellDamageGeometry::Build(RuntimeSpell.Definition));
    SyncEditableMeshScale();
    EditableMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    EditableMesh->SetMaterial(0, EarthVisualMaterial);
    bHasEditableMesh = true;
}

void AEarthSpellBody::ScaleEditableMesh(const float LinearFactor)
{
    if (!bHasEditableMesh || !EditableMesh) return;
    const double SafeFactor = FMath::Clamp(LinearFactor, 0.001f, 1.0f);
    EditableMesh->EditMesh(
        [SafeFactor](UE::Geometry::FDynamicMesh3& Mesh)
        {
            for (const int32 ID : Mesh.VertexIndicesItr())
            {
                Mesh.SetVertex(ID, Mesh.GetVertex(ID) * SafeFactor);
            }
        },
        EDynamicMeshComponentRenderUpdateMode::FullUpdate,
        EDynamicMeshChangeType::GeneralEdit,
        EDynamicMeshAttributeChangeFlags::Unknown);
}

bool AEarthSpellBody::ReceiveImpact_Implementation(const FImpactRequest& Request, FImpactResult& OutResult)
{
    if (!MaterialState || !Subtraction || !EditableMesh)
    {
        OutResult = FImpactResult();
        return false;
    }

    // Prevent both actors' OnHit delegates from processing one shared contact.
    // This suppresses duplicate transactions, not the actual Chaos collision.
    ImpactCooldownRemaining = FMath::Max(ImpactCooldownRemaining, 0.18f);

    FImpactRequest Canonical = Request;
    FVector Normal = Canonical.WorldSurfaceNormal.GetSafeNormal();
    const FVector Travel = Canonical.Source.VelocityCmS.GetSafeNormal();
    if (Normal.IsNearlyZero()) Normal = -Travel;
    if (!Travel.IsNearlyZero() && FVector::DotProduct(Travel, Normal) > 0.0f) Normal *= -1.0f;
    Canonical.WorldSurfaceNormal = Normal;

    OutResult = UImpactSolver::SolveSubtractiveImpact(
        Canonical, MaterialState->GetMaterialState(), ImpactTuning);
    if (!OutResult.Target.bProducesRemoval) return false;

    EnsureEditableMesh();
    const bool bChanged = Subtraction->ApplySolvedSubtraction(EditableMesh, Canonical, OutResult);
    if (!bChanged) return false;

    // The model now has a physically meaningful loss of material, not just a
    // visual scar. Approximate volume by the solver's locally removed volume.
    // Exact volume integration can replace this without changing our contracts.
    const float OldVolume = GetBodyVolumeM3();
    const float EstimatedLoss = FMath::Min(OutResult.Target.RemovedVolumeM3, OldVolume * 0.95f);
    const float Fraction = FMath::Clamp(1.0f - EstimatedLoss / FMath::Max(OldVolume, 0.000001f), 0.05f, 1.0f);
    SolidVolumeM3 = FMath::Max(OldVolume * Fraction, 0.000001f);
    RuntimeSpell.Body.VolumeM3 = SolidVolumeM3;
    RuntimeSpell.Body.MassKg = FMath::Max(RuntimeSpell.Body.MassKg * Fraction, 0.01f);
    Spell.MassKg = RuntimeSpell.Body.MassKg;
    RefreshMaterialState();
    RefreshPhysicalMaterial();
    if (BodyMesh && BodyMesh->IsSimulatingPhysics())
    {
        BodyMesh->SetMassOverrideInKg(NAME_None, GetBodyMassKg(), true);
    }

    BodyMesh->SetVisibility(false, false);
    EditableMesh->SetVisibility(true, false);
    return true;
}

void AEarthSpellBody::ScaleBodyAfterDamage(const float RemainingVolumeFraction)
{
    const float Fraction = FMath::Clamp(RemainingVolumeFraction, 0.001f, 1.0f);
    const float LinearScale = FMath::Pow(Fraction, 1.0f / 3.0f);
    FSpellDefinition& Definition = RuntimeSpell.Definition;
    switch (Definition.Shape)
    {
        case ESpellShape::Sphere:
            Definition.ShapeDefinition.SphereRadiusCm = FMath::Max(
                Definition.ShapeDefinition.SphereRadiusCm * LinearScale, 1.0f);
            break;
        case ESpellShape::Cube:
            Definition.ShapeDefinition.CubeXcm = FMath::Max(
                Definition.ShapeDefinition.CubeXcm * LinearScale, 1.0f);
            Definition.ShapeDefinition.CubeYcm = FMath::Max(
                Definition.ShapeDefinition.CubeYcm * LinearScale, 1.0f);
            Definition.ShapeDefinition.CubeZcm = FMath::Max(
                Definition.ShapeDefinition.CubeZcm * LinearScale, 1.0f);
            break;
        case ESpellShape::Cone:
        default:
            Definition.ShapeDefinition.ConeRadiusCm = FMath::Max(
                Definition.ShapeDefinition.ConeRadiusCm * LinearScale, 1.0f);
            Definition.ShapeDefinition.ConeHeightCm = FMath::Max(
                Definition.ShapeDefinition.ConeHeightCm * LinearScale, 1.0f);
            break;
    }
    const float OldVolume = GetBodyVolumeM3();
    SolidVolumeM3 = FMath::Max(OldVolume * Fraction, 0.000001f);
    RuntimeSpell.Body.VolumeM3 = SolidVolumeM3;
    RuntimeSpell.Body.MassKg = FMath::Max(RuntimeSpell.Body.MassKg * Fraction, 0.01f);
    RuntimeSpell.Body.ContactRadiusCm = FSpellShapeMath::CalculateContactRadiusCm(Definition);
    Spell = FSpellDefinitionAdapter::ToLegacyEarth(Definition);
    Spell.MassKg = RuntimeSpell.Body.MassKg;
    ScaleEditableMesh(LinearScale);
    RefreshMaterialState();
}

void AEarthSpellBody::ApplySourceOutcome(
    const FImpactResult& Result,
    const FHitResult& Hit,
    const bool bGeometryChanged)
{
    if (!Result.Source.bSurvives)
    {
        BodyMesh->SetSimulatePhysics(false);
        BodyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        SetLifeSpan(0.08f);
        return;
    }

    BodyMesh->SetSimulatePhysics(false);
    ScaleBodyAfterDamage(Result.Source.RemainingVolumeFraction);
    RebuildBodyGeometry();
    RefreshPhysicalMaterial();

    const float SeparationRadiusCm = FMath::Max(RuntimeSpell.Body.ContactRadiusCm, 2.0f);
    FVector SurfaceNormal = Result.Contact.SurfaceNormal.GetSafeNormal();
    if (SurfaceNormal.IsNearlyZero()) SurfaceNormal = Hit.ImpactNormal.GetSafeNormal();
    if (SurfaceNormal.IsNearlyZero()) SurfaceNormal = FVector::UpVector;

    if (Result.Motion.bContinuesForward && bGeometryChanged && !Result.Contact.IncomingDirection.IsNearlyZero())
    {
        const float AdvanceCm = FMath::Clamp(
            SeparationRadiusCm * 0.5f,
            2.0f,
            FMath::Max(Result.Target.PenetrationDepthCm * 0.35f, 2.0f));
        SetActorLocation(
            Hit.ImpactPoint + Result.Contact.IncomingDirection * AdvanceCm,
            false, nullptr, ETeleportType::TeleportPhysics);
    }
    else
    {
        SetActorLocation(
            Hit.ImpactPoint + SurfaceNormal * (SeparationRadiusCm + 1.5f),
            false, nullptr, ETeleportType::TeleportPhysics);
    }

    BodyMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    BodyMesh->SetEnableGravity(RuntimeSpell.Body.bAffectedByGravity);
    BodyMesh->SetSimulatePhysics(true);
    BodyMesh->SetMassOverrideInKg(NAME_None, GetBodyMassKg(), true);
    BodyMesh->SetPhysicsLinearVelocity(Result.Motion.PostImpactVelocityCmS);
    BodyMesh->WakeAllRigidBodies();
    LastFreeVelocityCmS = Result.Motion.PostImpactVelocityCmS;
    ImpactCooldownRemaining = 0.15f;
    // Surviving Earth persists after the collision.
}

void AEarthSpellBody::TryActivateExactSettledCollision(float DeltaSeconds)
{
    if (!bHasEditableMesh || bUsesExactSettledCollision || !BodyMesh || !EditableMesh)
    {
        return;
    }
    if (ImpactCooldownRemaining > 0.0f)
    {
        SettledSeconds = 0.0f;
        return;
    }
    // Chaos cannot simulate an arbitrarily cut triangle mesh as a dynamic
    // rigid body. Once a body settles, transition to the exact static collider.
    const bool bAtRest = BodyMesh->IsSimulatingPhysics()
        && !BodyMesh->RigidBodyIsAwake(NAME_None)
        && BodyMesh->GetPhysicsLinearVelocity().SizeSquared() < FMath::Square(8.0f);
    SettledSeconds = bAtRest ? SettledSeconds + DeltaSeconds : 0.0f;
    if (SettledSeconds < 0.75f) return;

    BodyMesh->SetSimulatePhysics(false);
    BodyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    EditableMesh->SetComplexAsSimpleCollisionEnabled(true, true);
    EditableMesh->SetDeferredCollisionUpdatesEnabled(false, true);
    EditableMesh->SetCollisionObjectType(ECC_WorldStatic);
    EditableMesh->SetCollisionResponseToAllChannels(ECR_Block);
    EditableMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
    EditableMesh->SetNotifyRigidBodyCollision(true);
    EditableMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    EditableMesh->UpdateCollision(false);
    bUsesExactSettledCollision = true;
}

void AEarthSpellBody::HandleBodyHit(
    UPrimitiveComponent* HitComponent,
    AActor* OtherActor,
    UPrimitiveComponent* OtherComponent,
    FVector NormalImpulse,
    const FHitResult& Hit)
{
    if (ImpactCooldownRemaining > 0.0f || !OtherActor || OtherActor == this
        || !OtherActor->GetClass()->ImplementsInterface(UImpactReceiver::StaticClass()))
    {
        return;
    }

    const AEarthSpellBody* OtherEarth = Cast<AEarthSpellBody>(OtherActor);
    if (OtherEarth && OtherEarth->ImpactCooldownRemaining > 0.0f)
    {
        return;
    }

    FImpactRequest Request;
    BuildImpactRequest_Implementation(Hit, Request);

    // Impact evaluates LOCAL relative motion when two free Earth bodies meet.
    // For the existing anchored test cube, target velocity is just zero.
    FVector TargetVelocity = FVector::ZeroVector;
    if (OtherEarth)
    {
        TargetVelocity = OtherEarth->GetIncomingVelocityCmS();
        Request.Source.VelocityCmS -= TargetVelocity;
    }

    FVector Normal = Request.WorldSurfaceNormal.GetSafeNormal();
    const FVector Travel = Request.Source.VelocityCmS.GetSafeNormal();
    if (Normal.IsNearlyZero()) Normal = -Travel;
    if (!Travel.IsNearlyZero() && FVector::DotProduct(Travel, Normal) > 0.0f) Normal *= -1.0f;
    Request.WorldSurfaceNormal = Normal;

    const float NormalSpeedMps = FMath::Max(0.0f,
        -FVector::DotProduct(Request.Source.VelocityCmS, Normal) * 0.01f);
    if (NormalSpeedMps < 0.75f) return;

    ImpactCooldownRemaining = 0.18f;
    FImpactResult Result;
    const bool bGeometryChanged = IImpactReceiver::Execute_ReceiveImpact(OtherActor, Request, Result);
    // Motion is calculated in the target frame; return to world-frame velocity.
    if (OtherEarth)
    {
        Result.Motion.PostImpactVelocityCmS += TargetVelocity;
    }

    if (GEngine)
    {
        const FString Message = FString::Printf(
            TEXT("Earth -> %s | %0.1fkJ | remove %0.3fm3 | r%0.0f d%0.0f | source %0.0f%% | %s"),
            *OtherActor->GetName(),
            Result.Energy.KineticEnergyJ / 1000.0f,
            Result.Target.RemovedVolumeM3,
            Result.Target.CraterRadiusCm,
            Result.Target.PenetrationDepthCm,
            Result.Source.Integrity01 * 100.0f,
            bGeometryChanged ? TEXT("TARGET DEFORMED") : TEXT("NO SUBTRACTION"));
        GEngine->AddOnScreenDebugMessage(-1, 4.0f, FColor::Yellow, Message);
    }
    ApplySourceOutcome(Result, Hit, bGeometryChanged);
}
