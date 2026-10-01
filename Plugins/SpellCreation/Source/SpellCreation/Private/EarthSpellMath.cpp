#include "EarthSpellMath.h"

#include "SpellDefinition.h"
#include "SpellShapeMath.h"

float UEarthSpellMath::CalculateVolumeM3(const FEarthSpellDefinition& Spell)
{
    return FSpellShapeMath::CalculateVolumeM3(FSpellDefinitionAdapter::FromLegacyEarth(Spell));
}

float UEarthSpellMath::CalculateDensityKgPerM3(const FEarthSpellDefinition& Spell)
{
    return FSpellShapeMath::CalculateDensityKgPerM3(FSpellDefinitionAdapter::FromLegacyEarth(Spell));
}

float UEarthSpellMath::CalculateMassKg(const FEarthSpellDefinition& Spell)
{
    return FSpellShapeMath::CalculateMassKg(FSpellDefinitionAdapter::FromLegacyEarth(Spell));
}

float UEarthSpellMath::CalculateContactRadiusCm(const FEarthSpellDefinition& Spell)
{
    return FSpellShapeMath::CalculateContactRadiusCm(FSpellDefinitionAdapter::FromLegacyEarth(Spell));
}

FVector UEarthSpellMath::CalculateHalfExtentsCm(const FEarthSpellDefinition& Spell)
{
    return FSpellShapeMath::CalculateHalfExtentsCm(FSpellDefinitionAdapter::FromLegacyEarth(Spell));
}
