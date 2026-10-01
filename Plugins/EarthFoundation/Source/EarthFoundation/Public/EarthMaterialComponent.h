#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EarthMaterialDefinition.h"
#include "EarthMaterialComponent.generated.h"

/** Owns Earth state and adapts it to MaterialCore. Never calculates impacts or edits geometry. */
UCLASS(ClassGroup=(Earth), meta=(BlueprintSpawnableComponent))
class EARTHFOUNDATION_API UEarthMaterialComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UEarthMaterialComponent();
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Earth|Material")
    TObjectPtr<UEarthMaterialDefinition> Definition;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Earth|Material", meta=(ShowOnlyInnerProperties))
    FEarthPhysicalProperties InlineProperties;
    UFUNCTION(BlueprintPure, Category="Earth|Material") FEarthPhysicalProperties GetProperties() const;
    UFUNCTION(BlueprintPure, Category="Earth|Material") FMaterialPhysicalProperties GetMaterialState() const;
};
