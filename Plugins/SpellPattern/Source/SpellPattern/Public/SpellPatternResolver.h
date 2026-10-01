#pragma once

#include "CoreMinimal.h"
#include "SpellPatternTypes.h"

struct SPELLPATTERN_API FResolvedPatternInstance
{
    FVector Location = FVector::ZeroVector;
    FQuat PatternRotation = FQuat::Identity;
};

struct SPELLPATTERN_API FSpellPatternResolver
{
    static void ResolveInstances(
        const FSpellPatternDefinition& Pattern,
        const FVector& PatternCenter,
        const FVector& CastForward,
        TArray<FResolvedPatternInstance>& OutInstances);

    static void ResolveLocations(
        const FSpellPatternDefinition& Pattern,
        const FVector& PatternCenter,
        const FVector& CastForward,
        TArray<FVector>& OutLocations);
};
