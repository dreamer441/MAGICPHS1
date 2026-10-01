#pragma once

#include "CoreMinimal.h"
#include "SpellMotionTypes.h"
#include "SpellPatternTypes.h"

/**
 * Generic motion-language resolver.
 *
 * It knows about direction vocabulary and pattern geometry only. It does not
 * know Earth, physics bodies, UI, damage, or key bindings.
 */
struct SPELLMOTION_API FSpellMotionResolver
{
    static FVector ResolveDirection(
        ESpellMotionDirection Direction,
        const FSpellPatternDefinition& Pattern,
        const FVector& InstanceLocation,
        const FVector& PatternCenter,
        const FVector& CastForward);
};
