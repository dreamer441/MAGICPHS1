#include "ImpactEnergySolver.h"

FImpactEnergyResult FImpactEnergySolver::Solve(
    const FImpactRequest& Request,
    const FMaterialPhysicalProperties& TargetMaterial,
    const FImpactSolverTuning& Tuning,
    const FImpactContactResult& Contact)
{
    FImpactEnergyResult Result;

    const float SourceHardness = FMath::Clamp(Request.Source.Material.Hardness, 0.0f, 1.0f);
    const float TargetHardness = FMath::Clamp(TargetMaterial.Hardness, 0.0f, 1.0f);
    const float TargetCohesion = FMath::Clamp(TargetMaterial.Cohesion, 0.0f, 1.0f);
    const float TargetDensity = FMath::Max(TargetMaterial.DensityKgPerM3, 1.0f);

    Result.SourceMassKg = FMath::Max(Request.Source.MassKg, 0.000001f);
    Result.KineticEnergyJ = 0.5f * Result.SourceMassKg * Contact.FullSpeedMps * Contact.FullSpeedMps;
    Result.NormalEnergyJ = 0.5f * Result.SourceMassKg * Contact.NormalSpeedMps * Contact.NormalSpeedMps;
    Result.TangentialEnergyJ = FMath::Max(Result.KineticEnergyJ - Result.NormalEnergyJ, 0.0f);

    // Hard sources couple normal impact energy into hard targets more efficiently.
    const float HardnessDenominator = FMath::Max(SourceHardness + TargetHardness, 0.05f);
    Result.TargetEnergyCoupling = FMath::Clamp(SourceHardness / HardnessDenominator, 0.05f, 0.95f);
    Result.TargetEffectiveEnergyJ = Result.NormalEnergyJ * Result.TargetEnergyCoupling;

    const float ReferenceDensity = FMath::Max(Tuning.ReferenceDensityKgPerM3, 1.0f);
    const float TargetDensityFactor = FMath::Clamp(FMath::Pow(TargetDensity / ReferenceDensity, 0.75f), 0.25f, 4.0f);
    const float TargetHardnessFactor = 0.25f + (2.75f * TargetHardness);
    const float TargetCohesionFactor = 0.50f + (0.50f * TargetCohesion);

    Result.TargetResistanceJPerM3 = FMath::Max(
        1.0f,
        Tuning.BaseResistanceJPerM3 * TargetDensityFactor * TargetHardnessFactor * TargetCohesionFactor);

    return Result;
}
