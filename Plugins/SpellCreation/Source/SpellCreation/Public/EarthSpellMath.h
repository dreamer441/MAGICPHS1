#pragma once

#include "CoreMinimal.h"
#include "EarthSpellDefinition.h"
#include "EarthSpellMath.generated.h"

/** Deprecated Blueprint math surface; delegates to generic FSpellShapeMath. */
UCLASS()
class SPELLCREATION_API UEarthSpellMath : public UObject
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintPure, Category="Spell|Math", meta=(DeprecatedFunction, DeprecationMessage="Use generic spell math"))
    static float CalculateVolumeM3(const FEarthSpellDefinition& Spell);

    UFUNCTION(BlueprintPure, Category="Spell|Math", meta=(DeprecatedFunction, DeprecationMessage="Use generic spell math"))
    static float CalculateDensityKgPerM3(const FEarthSpellDefinition& Spell);

    UFUNCTION(BlueprintPure, Category="Spell|Math", meta=(DeprecatedFunction, DeprecationMessage="Use generic spell math"))
    static float CalculateMassKg(const FEarthSpellDefinition& Spell);

    UFUNCTION(BlueprintPure, Category="Spell|Math", meta=(DeprecatedFunction, DeprecationMessage="Use generic spell math"))
    static float CalculateContactRadiusCm(const FEarthSpellDefinition& Spell);

    UFUNCTION(BlueprintPure, Category="Spell|Math", meta=(DeprecatedFunction, DeprecationMessage="Use generic spell math"))
    static FVector CalculateHalfExtentsCm(const FEarthSpellDefinition& Spell);
};
