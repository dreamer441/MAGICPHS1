#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ImpactTypes.h"
#include "ImpactSolver.generated.h"
UCLASS()
class IMPACTSYSTEM_API UImpactSolver : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintPure, Category="Impact|Solver")
    static FImpactResult SolveSubtractiveImpact(const FImpactRequest& Request, const FMaterialPhysicalProperties& TargetMaterial, const FImpactSolverTuning& Tuning);
};
