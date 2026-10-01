#pragma once

#include "CoreMinimal.h"
#include "ImpactTypes.h"

/** Pure contact node: extracts collision direction, incidence and contact area. */
class IMPACTSYSTEM_API FImpactContactSolver
{
public:
    static FImpactContactResult Solve(const FImpactRequest& Request);
};
