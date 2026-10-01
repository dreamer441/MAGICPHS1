#include "ImpactMotionSolver.h"

FImpactMotionResult FImpactMotionSolver::Solve(
    const FImpactRequest& Request,
    const FMaterialPhysicalProperties& TargetMaterial,
    const FImpactSolverTuning& Tuning,
    const FImpactContactResult& Contact,
    const FImpactEnergyResult& Energy,
    const FImpactTargetResult& Target,
    const FImpactSourceResult& Source)
{
    FImpactMotionResult Result;

    if (Energy.SourceMassKg <= KINDA_SMALL_NUMBER)
    {
        return Result;
    }

    const float SourceRestitution = FMath::Clamp(Request.Source.Material.Restitution, 0.0f, 1.0f);
    const float TargetRestitution = FMath::Clamp(TargetMaterial.Restitution, 0.0f, 1.0f);
    const float SourceRigidity = FMath::Clamp(Request.Source.Material.Rigidity, 0.0f, 1.0f);
    const float TargetRigidity = FMath::Clamp(TargetMaterial.Rigidity, 0.0f, 1.0f);
    const float TargetFriction = FMath::Clamp(TargetMaterial.Friction, 0.0f, 2.0f);

    // Material pair bounce tendency. Rigidity controls how efficiently that rebound is transmitted.
    Result.CombinedRestitution = FMath::Sqrt(SourceRestitution * TargetRestitution);
    const float RigidityTransfer = FMath::Sqrt(SourceRigidity * TargetRigidity);
    Result.EffectiveRestitution = FMath::Clamp(Result.CombinedRestitution * RigidityTransfer, 0.0f, 1.0f);

    if (!Source.bSurvives)
    {
        // No coherent source body remains. Its coherent post-impact momentum is zero,
        // so the whole incoming momentum is reported as transferred impulse.
        Result.TransferredImpulseNs = (Request.Source.VelocityCmS * 0.01f) * Energy.SourceMassKg;
        Result.EstimatedAverageForceN = Result.TransferredImpulseNs.Size()
            / FMath::Max(Tuning.EstimatedContactDurationS, 0.001f);
        Result.SpeedLossMps = Contact.FullSpeedMps;
        Result.bStops = true;
        return Result;
    }

    const float RemainingMassKg = FMath::Max(Energy.SourceMassKg * Source.RemainingVolumeFraction, 0.000001f);
    const float RemainingBlockedEnergyJ = FMath::Max(
        Energy.NormalEnergyJ - Target.AbsorbedEnergyJ - Source.DamageEnergyJ,
        0.0f);

    const float RemainingRadiusCm = Request.Source.ContactRadiusCm
        * FMath::Pow(FMath::Max(Source.RemainingVolumeFraction, 0.0f), 1.0f / 3.0f);

    const bool bChannelFits = Target.bProducesRemoval
        && Target.CraterRadiusCm >= RemainingRadiusCm * 0.90f
        && Target.PenetrationDepthCm >= RemainingRadiusCm * 1.25f;

    const float TangentialRetention = FMath::Clamp(1.0f - (0.35f * TargetFriction), 0.10f, 1.0f);
    const float TangentialRetainedEnergyJ = Energy.TangentialEnergyJ * TangentialRetention * TangentialRetention;

    FVector Direction = FVector::ZeroVector;
    float NormalRetainedEnergyJ = 0.0f;

    if (bChannelFits)
    {
        // If a real channel exists, the surviving core keeps travelling into it.
        // A very elastic/rigid pair still loses more forward energy to rebound stress.
        const float ForwardRetention = FMath::Clamp(0.85f - (0.35f * Result.EffectiveRestitution), 0.35f, 0.90f);
        NormalRetainedEnergyJ = RemainingBlockedEnergyJ * ForwardRetention;
        Direction = Contact.IncomingDirection;
        Result.bContinuesForward = true;
    }
    else
    {
        // No path through target: normal motion can only return as rebound.
        NormalRetainedEnergyJ = RemainingBlockedEnergyJ * Result.EffectiveRestitution * Result.EffectiveRestitution;

        FVector ReboundVector = Contact.SurfaceNormal * FMath::Sqrt(FMath::Max(NormalRetainedEnergyJ, 0.0f));
        if (!Contact.TangentialDirection.IsNearlyZero())
        {
            ReboundVector += Contact.TangentialDirection * FMath::Sqrt(FMath::Max(TangentialRetainedEnergyJ, 0.0f));
        }
        Direction = ReboundVector.GetSafeNormal();
        Result.bReflects = NormalRetainedEnergyJ > 1.0f;
    }

    Result.ResidualKineticEnergyJ = FMath::Max(NormalRetainedEnergyJ + TangentialRetainedEnergyJ, 0.0f);

    if (Result.ResidualKineticEnergyJ >= FMath::Max(Tuning.MinimumMotionEnergyJ, 0.0f))
    {
        Result.PostImpactSpeedMps = FMath::Sqrt((2.0f * Result.ResidualKineticEnergyJ) / RemainingMassKg);
        Result.PostImpactSpeedMps = FMath::Min(Result.PostImpactSpeedMps, Contact.FullSpeedMps);
    }

    if (Result.PostImpactSpeedMps < FMath::Max(Tuning.MinimumMotionSpeedMps, 0.0f) || Direction.IsNearlyZero())
    {
        Result.PostImpactSpeedMps = 0.0f;
        Result.ResidualKineticEnergyJ = 0.0f;
        Result.PostImpactVelocityCmS = FVector::ZeroVector;
        Result.bContinuesForward = false;
        Result.bReflects = false;
        Result.bStops = true;
    }
    else
    {
        Result.PostImpactVelocityCmS = Direction * (Result.PostImpactSpeedMps * 100.0f);
        Result.bStops = false;
    }

    Result.SpeedLossMps = FMath::Max(Contact.FullSpeedMps - Result.PostImpactSpeedMps, 0.0f);

    // Collision transfer is most naturally represented as impulse (N*s), not a single arbitrary "force" value.
    const FVector InitialMomentumKgMps = (Request.Source.VelocityCmS * 0.01f) * Energy.SourceMassKg;
    const FVector FinalMomentumKgMps = (Result.PostImpactVelocityCmS * 0.01f) * RemainingMassKg;
    Result.TransferredImpulseNs = InitialMomentumKgMps - FinalMomentumKgMps;
    Result.EstimatedAverageForceN = Result.TransferredImpulseNs.Size()
        / FMath::Max(Tuning.EstimatedContactDurationS, 0.001f);

    return Result;
}
