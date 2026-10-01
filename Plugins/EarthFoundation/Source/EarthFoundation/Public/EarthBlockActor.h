#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ImpactReceiver.h"
#include "ImpactTypes.h"
#include "EarthBlockActor.generated.h"
class UDynamicMeshComponent;
class UEarthGeometryComponent;
class UEarthMaterialComponent;
class UEarthSubtractionComponent;
class UMaterialInterface;
class UPhysicalMaterial;

/** Composition root: MaterialCore state -> ImpactSystem result -> Earth geometry response. */
UCLASS(Blueprintable)
class EARTHFOUNDATION_API AEarthBlockActor : public AActor, public IImpactReceiver
{
    GENERATED_BODY()
public:
    AEarthBlockActor();
    virtual void OnConstruction(const FTransform& Transform) override;
    virtual void BeginPlay() override;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Earth|Nodes") TObjectPtr<UDynamicMeshComponent> EarthMesh;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Earth|Nodes") TObjectPtr<UEarthMaterialComponent> MaterialState;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Earth|Nodes") TObjectPtr<UEarthGeometryComponent> Geometry;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Earth|Nodes") TObjectPtr<UEarthSubtractionComponent> Subtraction;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Earth|Impact") FImpactSolverTuning ImpactTuning;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category="Earth|Impact") FImpactResult LastImpactResult;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Earth|Visual") TObjectPtr<UMaterialInterface> VisualMaterial;
    UFUNCTION(BlueprintCallable, CallInEditor, Category="Earth|Debug") void RebuildEarthBlock();
    UFUNCTION(BlueprintCallable, Category="Earth|Visual") void SetEarthVisualMaterial(UMaterialInterface* NewMaterial);
    virtual bool ReceiveImpact_Implementation(const FImpactRequest& Request, FImpactResult& OutResult) override;
private:
    UPROPERTY(Transient) TObjectPtr<UPhysicalMaterial> RuntimePhysicalMaterial;
    void RefreshPhysicalMaterial();
};
