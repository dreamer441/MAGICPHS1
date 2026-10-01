#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "EarthPhysicalProperties.h"
#include "EarthMaterialDefinition.generated.h"
UCLASS(BlueprintType)
class EARTHFOUNDATION_API UEarthMaterialDefinition : public UPrimaryDataAsset
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Earth|Definition")
    FText DisplayName = FText::FromString(TEXT("Soil"));
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Earth|Definition", meta=(ShowOnlyInnerProperties))
    FEarthPhysicalProperties Properties;
};
