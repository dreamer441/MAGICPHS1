#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EarthGeometryComponent.generated.h"
class UDynamicMeshComponent;

/** Owns only Earth block geometry creation/reset. No impact physics or material calculations. */
UCLASS(ClassGroup=(Earth), meta=(BlueprintSpawnableComponent))
class EARTHFOUNDATION_API UEarthGeometryComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UEarthGeometryComponent();
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Earth|Geometry") FVector BlockSizeCm = FVector(2000.0, 2000.0, 300.0);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Earth|Geometry", meta=(ClampMin="5", ClampMax="101")) int32 SurfaceResolution = 41;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Earth|Geometry", meta=(ClampMin="2", ClampMax="101")) int32 VerticalResolution = 9;
    UFUNCTION(BlueprintCallable, Category="Earth|Geometry") void RebuildBlock(UDynamicMeshComponent* TargetMesh);
    FVector GetResolvedBlockSizeCm() const { return ResolvedBlockSizeCm; }
private:
    FVector ResolvedBlockSizeCm = FVector(2000.0, 2000.0, 300.0);
};
