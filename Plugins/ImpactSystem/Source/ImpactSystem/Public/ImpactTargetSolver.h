#pragma once

#include "CoreMinimal.h"
#include "ImpactTypes.h"

/** Pure target-response node. Converts energy into removal amount and spatial distribution. */
class IMPACTSYSTEM_API FImpactTargetSolver
{
public:
    static FImpactTargetResult Solve(
        const FImpactRequest& Request,
        const FMaterialPhysicalProperties& TargetMaterial,
        const FImpactSolverTuning& Tuning,
        const FImpactContactResult& Contact,
        const FImpactEnergyResult& Energy);
};
