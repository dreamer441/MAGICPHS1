#include "ImpactContactSolver.h"

FImpactContactResult FImpactContactSolver::Solve(const FImpactRequest& Request)
{
    FImpactContactResult Result;

    const FVector VelocityMps = Request.Source.VelocityCmS * 0.01f;
    Result.FullSpeedMps = VelocityMps.Size();
    Result.IncomingDirection = VelocityMps.GetSafeNormal();

    Result.SurfaceNormal = Request.WorldSurfaceNormal.GetSafeNormal();
    if (Result.SurfaceNormal.IsNearlyZero())
    {
        Result.SurfaceNormal = Result.IncomingDirection.IsNearlyZero()
            ? FVector::UpVector
            : -Result.IncomingDirection;
    }

    if (!Result.IncomingDirection.IsNearlyZero() && FVector::DotProduct(Result.IncomingDirection, Result.SurfaceNormal) > 0.0f)
    {
        Result.SurfaceNormal *= -1.0f;
    }

    Result.Incidence = Result.FullSpeedMps > KINDA_SMALL_NUMBER
        ? FMath::Clamp(FVector::DotProduct(Result.IncomingDirection, -Result.SurfaceNormal), 0.0f, 1.0f)
        : 0.0f;

    Result.NormalSpeedMps = Result.FullSpeedMps * Result.Incidence;
    Result.TangentialSpeedMps = FMath::Sqrt(FMath::Max(
        Result.FullSpeedMps * Result.FullSpeedMps - Result.NormalSpeedMps * Result.NormalSpeedMps,
        0.0f));

    const FVector NormalVelocityMps = -Result.SurfaceNormal * Result.NormalSpeedMps;
    const FVector TangentialVelocityMps = VelocityMps - NormalVelocityMps;
    Result.TangentialDirection = TangentialVelocityMps.GetSafeNormal();

    const float RadiusM = FMath::Max(Request.Source.ContactRadiusCm, 0.1f) * 0.01f;
    Result.ContactAreaM2 = PI * RadiusM * RadiusM;
    return Result;
}
