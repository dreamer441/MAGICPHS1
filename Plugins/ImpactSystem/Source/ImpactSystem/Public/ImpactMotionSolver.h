#pragma once

#include "CoreMinimal.h"
#include "ImpactTypes.h"

/**
 * Pure momentum/impulse node.
 * Converts the already-computed material response into post-impact source motion and transferred impulse.
 * It never moves an actor and never edits target geometry.
 */
class IMPACTSYSTEM_API FImpactMotionSolver
{
public:
    static FImpactMotionResult Solve(
        const FImpactRequest& Request,
        const FMaterialPhysicalProperties& TargetMaterial,
        const FImpactSolverTuning& Tuning,
        const FImpactContactResult& Contact,
        const FImpactEnergyResult& Energy,
        const FImpactTargetResult& Target,
        const FImpactSourceResult& Source);
};
