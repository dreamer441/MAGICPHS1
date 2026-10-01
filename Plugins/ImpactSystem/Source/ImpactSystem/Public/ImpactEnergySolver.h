#pragma once

#include "CoreMinimal.h"
#include "ImpactTypes.h"

/** Pure energy-accounting node for an impact. No target geometry or projectile visuals. */
class IMPACTSYSTEM_API FImpactEnergySolver
{
public:
    static FImpactEnergyResult Solve(
        const FImpactRequest& Request,
        const FMaterialPhysicalProperties& TargetMaterial,
        const FImpactSolverTuning& Tuning,
        const FImpactContactResult& Contact);
};
