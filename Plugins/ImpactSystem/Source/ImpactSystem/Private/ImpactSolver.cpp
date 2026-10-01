#include "ImpactSolver.h"

#include "ImpactContactSolver.h"
#include "ImpactEnergySolver.h"
#include "ImpactMotionSolver.h"
#include "ImpactSourceSolver.h"
#include "ImpactTargetSolver.h"

FImpactResult UImpactSolver::SolveSubtractiveImpact(
    const FImpactRequest& Request,
    const FMaterialPhysicalProperties& TargetMaterial,
    const FImpactSolverTuning& Tuning)
{
    FImpactResult Result;

    // Orchestrator only. Each node owns one question and passes data forward.
    Result.Contact = FImpactContactSolver::Solve(Request);
    Result.Energy = FImpactEnergySolver::Solve(Request, TargetMaterial, Tuning, Result.Contact);
    Result.Target = FImpactTargetSolver::Solve(Request, TargetMaterial, Tuning, Result.Contact, Result.Energy);
    Result.Source = FImpactSourceSolver::Solve(Request, TargetMaterial, Tuning, Result.Energy, Result.Target);
    Result.Motion = FImpactMotionSolver::Solve(Request, TargetMaterial, Tuning, Result.Contact, Result.Energy, Result.Target, Result.Source);

    return Result;
}
