#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PhysicalBodyMath.generated.h"

/** Stateless conversion helpers only. */
UCLASS()
class PHYSICALBODY_API UPhysicalBodyMath : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintPure, Category="Physical Body|Math")
    static float MassFromDensityAndVolume(float DensityKgPerM3, float VolumeM3);

    UFUNCTION(BlueprintPure, Category="Physical Body|Math")
    static float DensityFromMassAndVolume(float MassKg, float VolumeM3);
};
