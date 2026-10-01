#pragma once

#include "CoreMinimal.h"
#include "SpellDefinition.h"

class APlayerController;

/** Character-relative realization transform separated from camera-derived aim. */
struct SPELLEXECUTION_API FResolvedSpellCastPlacement
{
    FVector SpawnLocation = FVector::ZeroVector;
    FVector LaunchDirection = FVector::ForwardVector;
    FRotator SpawnRotation = FRotator::ZeroRotator;
};

/** Computes a safe character-relative spawn placement for any resolved spell. */
struct SPELLEXECUTION_API FSpellCastPlacement
{
    static bool Resolve(
        APlayerController* PlayerController,
        const FResolvedSpell& Spell,
        const FVector& DesiredAimDirection,
        FResolvedSpellCastPlacement& OutPlacement);
};
