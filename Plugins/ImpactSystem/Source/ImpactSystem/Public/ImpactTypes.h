#pragma once

#include "CoreMinimal.h"
#include "MaterialPhysicalProperties.h"
#include "PhysicalBodyState.h"
#include "ImpactTypes.generated.h"

/** Geometry-neutral collision request. Source and target never reach into each other's implementation. */
USTRUCT(BlueprintType)
struct IMPACTSYSTEM_API FImpactRequest
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Impact")
    FVector WorldPosition = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Impact")
    FVector WorldSurfaceNormal = FVector::UpVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Impact")
    FPhysicalBodyState Source;
};

/** Tunable constants for the generic impact model. Material data itself belongs in MaterialCore. */
USTRUCT(BlueprintType)
struct IMPACTSYSTEM_API FImpactSolverTuning
{
    GENERATED_BODY()

    // TARGET AMOUNT
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Impact|Solver|Target Amount", meta=(ClampMin="1.0"))
    float BaseResistanceJPerM3 = 120000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Impact|Solver|Target Amount", meta=(ClampMin="1.0"))
    float ReferenceDensityKgPerM3 = 1600.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Impact|Solver|Target Amount", meta=(ClampMin="0.0001"))
    float MaxRemovedVolumeM3 = 3.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Impact|Solver|Target Amount", meta=(ClampMin="0.0"))
    float MinimumEffectiveEnergyJ = 25.0f;

    // TARGET SHAPE
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Impact|Solver|Target Shape", meta=(ClampMin="0.1"))
    float ReferenceSpeedMps = 30.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Impact|Solver|Target Shape", meta=(ClampMin="0.1"))
    float ReferenceContactRadiusCm = 22.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Impact|Solver|Target Shape", meta=(ClampMin="0.05"))
    float MinPenetrationAspectRatio = 0.25f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Impact|Solver|Target Shape", meta=(ClampMin="0.1"))
    float MaxPenetrationAspectRatio = 8.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Impact|Solver|Target Shape", meta=(ClampMin="1.0"))
    float MaximumCraterRadiusCm = 180.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Impact|Solver|Target Shape", meta=(ClampMin="1.0"))
    float MaximumPenetrationDepthCm = 300.0f;

    // SOURCE STRUCTURE
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Impact|Solver|Source Structure", meta=(ClampMin="1000.0"))
    float BaseSourceFractureEnergyJPerM3 = 350000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Impact|Solver|Source Structure", meta=(ClampMin="0.0", ClampMax="1.0"))
    float MinimumSurvivalIntegrity = 0.08f;

    // MOTION / IMPULSE
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Impact|Solver|Motion", meta=(ClampMin="0.001", ClampMax="0.2", Units="s"))
    float EstimatedContactDurationS = 0.02f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Impact|Solver|Motion", meta=(ClampMin="0.0"))
    float MinimumMotionEnergyJ = 20.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Impact|Solver|Motion", meta=(ClampMin="0.0"))
    float MinimumMotionSpeedMps = 0.50f;
};

/** Contact node: purely geometric/kinematic facts about the collision. */
USTRUCT(BlueprintType)
struct IMPACTSYSTEM_API FImpactContactResult
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Impact|Contact") FVector SurfaceNormal = FVector::UpVector;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Impact|Contact") FVector IncomingDirection = FVector::ForwardVector;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Impact|Contact") FVector TangentialDirection = FVector::ZeroVector;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Impact|Contact") float FullSpeedMps = 0.0f;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Impact|Contact") float NormalSpeedMps = 0.0f;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Impact|Contact") float TangentialSpeedMps = 0.0f;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Impact|Contact") float Incidence = 0.0f;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Impact|Contact") float ContactAreaM2 = 0.0f;
};

/** Energy accounting node. No geometry instructions live here. */
USTRUCT(BlueprintType)
struct IMPACTSYSTEM_API FImpactEnergyResult
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Impact|Energy") float SourceMassKg = 0.0f;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Impact|Energy") float KineticEnergyJ = 0.0f;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Impact|Energy") float NormalEnergyJ = 0.0f;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Impact|Energy") float TangentialEnergyJ = 0.0f;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Impact|Energy") float TargetEnergyCoupling = 0.0f;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Impact|Energy") float TargetEffectiveEnergyJ = 0.0f;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Impact|Energy") float TargetResistanceJPerM3 = 0.0f;
};

/** Target-side result. It describes what should happen; it does not edit a mesh. */
USTRUCT(BlueprintType)
struct IMPACTSYSTEM_API FImpactTargetResult
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Impact|Target") bool bProducesRemoval = false;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Impact|Target") float AbsorbedEnergyJ = 0.0f;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Impact|Target") float RemovedVolumeM3 = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Impact|Target|Shape") float ContactAreaM2 = 0.0f;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Impact|Target|Shape") float PenetrationDrive = 0.0f;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Impact|Target|Shape") float LateralSpreadDrive = 0.0f;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Impact|Target|Shape") float PenetrationAspectRatio = 1.0f;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Impact|Target|Shape") float CraterRadiusCm = 0.0f;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Impact|Target|Shape") float PenetrationDepthCm = 0.0f;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Impact|Target|Shape") FVector WorldPenetrationDirection = FVector::ForwardVector;
};

/** Source structural result. It answers only "how much of the source remains intact?" */
USTRUCT(BlueprintType)
struct IMPACTSYSTEM_API FImpactSourceResult
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Impact|Source") float BlockedEnergyJ = 0.0f;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Impact|Source") float FractureCapacityJ = 0.0f;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Impact|Source") float DamageEnergyJ = 0.0f;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Impact|Source", meta=(ClampMin="0.0", ClampMax="1.0")) float Integrity01 = 1.0f;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Impact|Source", meta=(ClampMin="0.0", ClampMax="1.0")) float RemainingVolumeFraction = 1.0f;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Impact|Source") bool bSurvives = true;
};

/** Momentum/impulse result. It answers only "how does the surviving body move after impact?" */
USTRUCT(BlueprintType)
struct IMPACTSYSTEM_API FImpactMotionResult
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Impact|Motion") float CombinedRestitution = 0.0f;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Impact|Motion") float EffectiveRestitution = 0.0f;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Impact|Motion") float ResidualKineticEnergyJ = 0.0f;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Impact|Motion") float PostImpactSpeedMps = 0.0f;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Impact|Motion") float SpeedLossMps = 0.0f;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Impact|Motion") FVector PostImpactVelocityCmS = FVector::ZeroVector;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Impact|Motion") FVector TransferredImpulseNs = FVector::ZeroVector;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Impact|Motion") float EstimatedAverageForceN = 0.0f;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Impact|Motion") bool bContinuesForward = false;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Impact|Motion") bool bReflects = false;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Impact|Motion") bool bStops = true;
};

/** Complete collision result. Each child is produced by one independent solver node. */
USTRUCT(BlueprintType)
struct IMPACTSYSTEM_API FImpactResult
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Impact|Result") FImpactContactResult Contact;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Impact|Result") FImpactEnergyResult Energy;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Impact|Result") FImpactTargetResult Target;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Impact|Result") FImpactSourceResult Source;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Impact|Result") FImpactMotionResult Motion;
};
