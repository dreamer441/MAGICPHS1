#include "ImpactTargetSolver.h"

namespace ImpactTargetShape
{
    constexpr float SpeedExponent = 1.10f;
    constexpr float DensityExponent = 0.55f;
    constexpr float ContactExponent = 0.75f;
    constexpr float MinDimensionCm = 0.5f;

    static float SafePow(float Base, float Exponent)
    {
        return FMath::Pow(FMath::Max(Base, 0.0001f), Exponent);
    }

    // For a half-ellipsoid crater whose center is on the surface:
    // V = (2/3) * PI * Radius^2 * Depth.
    static void DimensionsFromVolumeAndAspect(float VolumeM3, float AspectRatio, float& OutRadiusCm, float& OutDepthCm)
    {
        const double VolumeCm3 = static_cast<double>(FMath::Max(VolumeM3, 0.0f)) * 1000000.0;
        const double SafeAspect = FMath::Max(static_cast<double>(AspectRatio), 0.0001);
        const double RadiusCm = FMath::Pow((3.0 * VolumeCm3) / (2.0 * PI * SafeAspect), 1.0 / 3.0);
        OutRadiusCm = static_cast<float>(RadiusCm);
        OutDepthCm = static_cast<float>(RadiusCm * SafeAspect);
    }

    static float DepthForVolumeAndRadius(float VolumeM3, float RadiusCm)
    {
        const double VolumeCm3 = static_cast<double>(FMath::Max(VolumeM3, 0.0f)) * 1000000.0;
        const double SafeRadius = FMath::Max(static_cast<double>(RadiusCm), 0.001);
        return static_cast<float>((3.0 * VolumeCm3) / (2.0 * PI * SafeRadius * SafeRadius));
    }

    static float RadiusForVolumeAndDepth(float VolumeM3, float DepthCm)
    {
        const double VolumeCm3 = static_cast<double>(FMath::Max(VolumeM3, 0.0f)) * 1000000.0;
        const double SafeDepth = FMath::Max(static_cast<double>(DepthCm), 0.001);
        return static_cast<float>(FMath::Sqrt((3.0 * VolumeCm3) / (2.0 * PI * SafeDepth)));
    }
}

FImpactTargetResult FImpactTargetSolver::Solve(
    const FImpactRequest& Request,
    const FMaterialPhysicalProperties& TargetMaterial,
    const FImpactSolverTuning& Tuning,
    const FImpactContactResult& Contact,
    const FImpactEnergyResult& Energy)
{
    FImpactTargetResult Result;

    const float SourceDensity = FMath::Max(Request.Source.Material.DensityKgPerM3, 1.0f);
    const float SourceHardness = FMath::Clamp(Request.Source.Material.Hardness, 0.0f, 1.0f);
    const float TargetHardness = FMath::Clamp(TargetMaterial.Hardness, 0.0f, 1.0f);
    const float TargetCohesion = FMath::Clamp(TargetMaterial.Cohesion, 0.0f, 1.0f);
    const float TargetDensity = FMath::Max(TargetMaterial.DensityKgPerM3, 1.0f);
    const float SourceContactRadiusCm = FMath::Max(Request.Source.ContactRadiusCm, 0.1f);

    if (Energy.TargetEffectiveEnergyJ < FMath::Max(Tuning.MinimumEffectiveEnergyJ, 0.0f))
    {
        return Result;
    }

    Result.RemovedVolumeM3 = FMath::Clamp(
        Energy.TargetEffectiveEnergyJ / FMath::Max(Energy.TargetResistanceJPerM3, 1.0f),
        0.0f,
        FMath::Max(Tuning.MaxRemovedVolumeM3, 0.0001f));

    if (Result.RemovedVolumeM3 <= KINDA_SMALL_NUMBER)
    {
        return Result;
    }

    // The target can only absorb the amount of work represented by the removal it actually performs.
    Result.AbsorbedEnergyJ = FMath::Min(
        Energy.TargetEffectiveEnergyJ,
        Result.RemovedVolumeM3 * FMath::Max(Energy.TargetResistanceJPerM3, 1.0f));

    const float ContactRadiusM = SourceContactRadiusCm * 0.01f;
    Result.ContactAreaM2 = Contact.ContactAreaM2;

    const float ReferenceSpeed = FMath::Max(Tuning.ReferenceSpeedMps, 0.1f);
    const float ReferenceContactRadius = FMath::Max(Tuning.ReferenceContactRadiusCm, 0.1f);

    const float SpeedDrive = ImpactTargetShape::SafePow(
        FMath::Max(Contact.NormalSpeedMps / ReferenceSpeed, 0.05f),
        ImpactTargetShape::SpeedExponent);

    const float DensityDrive = ImpactTargetShape::SafePow(
        FMath::Clamp(SourceDensity / TargetDensity, 0.10f, 10.0f),
        ImpactTargetShape::DensityExponent);

    const float HardnessDrive = FMath::Clamp(
        (0.25f + 0.75f * SourceHardness) / FMath::Max(0.25f + 0.75f * TargetHardness, 0.05f),
        0.25f,
        4.0f);

    const float SizeSpread = ImpactTargetShape::SafePow(
        FMath::Max(SourceContactRadiusCm / ReferenceContactRadius, 0.05f),
        ImpactTargetShape::ContactExponent);

    const float CohesionSpread = FMath::Lerp(1.35f, 0.75f, TargetCohesion);

    Result.PenetrationDrive = SpeedDrive * DensityDrive * HardnessDrive;
    Result.LateralSpreadDrive = FMath::Max(SizeSpread * CohesionSpread, 0.05f);

    Result.PenetrationAspectRatio = FMath::Clamp(
        Result.PenetrationDrive / Result.LateralSpreadDrive,
        FMath::Max(Tuning.MinPenetrationAspectRatio, 0.05f),
        FMath::Max(Tuning.MaxPenetrationAspectRatio, Tuning.MinPenetrationAspectRatio));

    ImpactTargetShape::DimensionsFromVolumeAndAspect(
        Result.RemovedVolumeM3,
        Result.PenetrationAspectRatio,
        Result.CraterRadiusCm,
        Result.PenetrationDepthCm);

    const float MinimumFootprintRadiusCm = SourceContactRadiusCm * 0.35f;
    if (Result.CraterRadiusCm < MinimumFootprintRadiusCm)
    {
        Result.CraterRadiusCm = MinimumFootprintRadiusCm;
        Result.PenetrationDepthCm = ImpactTargetShape::DepthForVolumeAndRadius(Result.RemovedVolumeM3, Result.CraterRadiusCm);
    }

    const float MaxDepth = FMath::Max(Tuning.MaximumPenetrationDepthCm, ImpactTargetShape::MinDimensionCm);
    if (Result.PenetrationDepthCm > MaxDepth)
    {
        Result.PenetrationDepthCm = MaxDepth;
        Result.CraterRadiusCm = ImpactTargetShape::RadiusForVolumeAndDepth(Result.RemovedVolumeM3, Result.PenetrationDepthCm);
    }

    const float MaxRadius = FMath::Max(Tuning.MaximumCraterRadiusCm, ImpactTargetShape::MinDimensionCm);
    if (Result.CraterRadiusCm > MaxRadius)
    {
        Result.CraterRadiusCm = MaxRadius;
        Result.PenetrationDepthCm = ImpactTargetShape::DepthForVolumeAndRadius(Result.RemovedVolumeM3, Result.CraterRadiusCm);
    }

    Result.CraterRadiusCm = FMath::Max(Result.CraterRadiusCm, ImpactTargetShape::MinDimensionCm);
    Result.PenetrationDepthCm = FMath::Max(Result.PenetrationDepthCm, ImpactTargetShape::MinDimensionCm);
    Result.PenetrationAspectRatio = Result.PenetrationDepthCm / FMath::Max(Result.CraterRadiusCm, ImpactTargetShape::MinDimensionCm);

    const FVector VelocityDirection = Request.Source.VelocityCmS.GetSafeNormal();
    FVector SurfaceNormal = Request.WorldSurfaceNormal.GetSafeNormal();
    if (SurfaceNormal.IsNearlyZero()) SurfaceNormal = FVector::UpVector;

    Result.WorldPenetrationDirection = !VelocityDirection.IsNearlyZero() ? VelocityDirection : -SurfaceNormal;
    if (FVector::DotProduct(Result.WorldPenetrationDirection, SurfaceNormal) > 0.0f)
    {
        Result.WorldPenetrationDirection *= -1.0f;
    }

    Result.bProducesRemoval = true;
    return Result;
}
