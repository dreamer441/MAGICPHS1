#pragma once

#include "CoreMinimal.h"
#include "ImpactTypes.h"

/** Pure source-structure node. Computes fracture/survival only; it never mutates or moves an actor. */
class IMPACTSYSTEM_API FImpactSourceSolver
{
public:
    static FImpactSourceResult Solve(
        const FImpactRequest& Request,
        const FMaterialPhysicalProperties& TargetMaterial,
        const FImpactSolverTuning& Tuning,
        const FImpactEnergyResult& Energy,
        const FImpactTargetResult& Target);
};
