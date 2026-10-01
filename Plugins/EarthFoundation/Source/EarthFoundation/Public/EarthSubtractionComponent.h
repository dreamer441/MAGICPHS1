#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ImpactTypes.h"
#include "EarthSubtractionComponent.generated.h"

class UDynamicMeshComponent;

/**
 * Earth-specific geometry adapter.
 * It receives a geometry-neutral FImpactResult and realizes the requested
 * crater radius/depth as an oriented ellipsoid subtraction.
 */
UCLASS(ClassGroup=(Earth), meta=(BlueprintSpawnableComponent))
class EARTHFOUNDATION_API UEarthSubtractionComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UEarthSubtractionComponent();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Earth|Subtraction", meta=(ClampMin="8", ClampMax="48"))
    int32 CutterResolution = 24;

    // Tiny inset keeps the boolean cutter safely intersecting the target surface.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Earth|Subtraction", meta=(ClampMin="0.0", ClampMax="5.0"))
    float SurfaceInsetCm = 0.75f;

    UFUNCTION(BlueprintCallable, Category="Earth|Subtraction")
    bool ApplySolvedSubtraction(UDynamicMeshComponent* TargetMesh, const FImpactRequest& Request, const FImpactResult& Result);
};
