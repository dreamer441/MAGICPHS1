#pragma once

#include "CoreMinimal.h"
#include "SpellDefinition.h"

struct SPELLCREATION_API FSpellShapeMath
{
    static float CalculateVolumeM3(const FSpellDefinition& Definition);
    static float CalculateDensityKgPerM3(const FSpellDefinition& Definition);
    static float CalculateMassKg(const FSpellDefinition& Definition);
    static float CalculateContactRadiusCm(const FSpellDefinition& Definition);
    static FVector CalculateHalfExtentsCm(const FSpellDefinition& Definition);
};
