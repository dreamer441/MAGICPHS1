#pragma once
#include "CoreMinimal.h"
#include "MaterialPhysicalProperties.h"
#include "EarthPhysicalProperties.generated.h"

/** Earth-specific state = generic MaterialCore state + Earth-only properties. */
USTRUCT(BlueprintType)
struct EARTHFOUNDATION_API FEarthPhysicalProperties
{
    GENERATED_BODY()

    FEarthPhysicalProperties()
    {
        Material.DensityKgPerM3 = 1600.0f;
        Material.Hardness = 0.25f;
        Material.Cohesion = 0.35f;
        Material.Plasticity = 0.65f;
        Material.Friction = 0.75f;
        Material.Restitution = 0.08f;
        Material.Toughness = 0.40f;
        Material.Rigidity = 0.55f;
        Material.TemperatureC = 15.0f;
    }

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Earth|Properties", meta=(ShowOnlyInnerProperties))
    FMaterialPhysicalProperties Material;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Earth|Properties", meta=(ClampMin="0.0", ClampMax="1.0"))
    float Moisture = 0.20f;
};
