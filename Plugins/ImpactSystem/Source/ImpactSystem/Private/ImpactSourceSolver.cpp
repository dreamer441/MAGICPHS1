#include "ImpactSourceSolver.h"

FImpactSourceResult FImpactSourceSolver::Solve(
    const FImpactRequest& Request,
    const FMaterialPhysicalProperties& TargetMaterial,
    const FImpactSolverTuning& Tuning,
    const FImpactEnergyResult& Energy,
    const FImpactTargetResult& Target)
{
    FImpactSourceResult Result;

    const float SourceVolumeM3 = FMath::Max(Request.Source.VolumeM3, 0.000001f);
    const float SourceDensity = FMath::Max(Request.Source.Material.DensityKgPerM3, 1.0f);
    const float SourceHardness = FMath::Clamp(Request.Source.Material.Hardness, 0.0f, 1.0f);
    const float SourceCohesion = FMath::Clamp(Request.Source.Material.Cohesion, 0.0f, 1.0f);
    const float SourceToughness = FMath::Clamp(Request.Source.Material.Toughness, 0.0f, 1.0f);
    const float SourceRigidity = FMath::Clamp(Request.Source.Material.Rigidity, 0.0f, 1.0f);
    const float TargetHardness = FMath::Clamp(TargetMaterial.Hardness, 0.0f, 1.0f);

    Result.BlockedEnergyJ = FMath::Max(Energy.NormalEnergyJ - Target.AbsorbedEnergyJ, 0.0f);

    const float DensityFactor = FMath::Clamp(
        FMath::Pow(SourceDensity / FMath::Max(Tuning.ReferenceDensityKgPerM3, 1.0f), 0.25f),
        0.5f,
        2.0f);

    // Toughness is the main anti-fracture property. Cohesion helps the body stay one piece.
    // Rigidity is not automatically "strong": high rigidity + low toughness becomes brittle.
    const float ToughnessCapacity = 0.25f + (2.75f * SourceToughness);
    const float CohesionCapacity = 0.55f + (1.25f * SourceCohesion);
    const float BrittlePenalty = 1.0f - (0.55f * SourceRigidity * (1.0f - SourceToughness));

    Result.FractureCapacityJ = FMath::Max(
        1.0f,
        Tuning.BaseSourceFractureEnergyJPerM3
        * SourceVolumeM3
        * DensityFactor
        * ToughnessCapacity
        * CohesionCapacity
        * FMath::Clamp(BrittlePenalty, 0.25f, 1.0f));

    // A harder target relative to the source increases source-side damage.
    const float HardnessMismatch = FMath::Clamp(
        (0.25f + 0.75f * TargetHardness) / FMath::Max(0.25f + 0.75f * SourceHardness, 0.05f),
        0.20f,
        5.0f);

    const float Brittleness = SourceRigidity * (1.0f - SourceToughness);
    const float DamageShare = FMath::Clamp(
        0.18f * HardnessMismatch * FMath::Lerp(0.75f, 1.75f, Brittleness),
        0.04f,
        0.95f);

    Result.DamageEnergyJ = Result.BlockedEnergyJ * DamageShare;
    Result.Integrity01 = FMath::Clamp(1.0f - (Result.DamageEnergyJ / Result.FractureCapacityJ), 0.0f, 1.0f);
    Result.RemainingVolumeFraction = FMath::Clamp(FMath::Pow(Result.Integrity01, 1.15f), 0.0f, 1.0f);
    Result.bSurvives = Result.Integrity01 >= FMath::Clamp(Tuning.MinimumSurvivalIntegrity, 0.0f, 1.0f)
        && Result.RemainingVolumeFraction > 0.001f;

    return Result;
}
