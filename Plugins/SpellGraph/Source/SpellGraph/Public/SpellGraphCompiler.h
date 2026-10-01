#pragma once

#include "CoreMinimal.h"
#include "SpellGraphTypes.h"

class UWorldCodexSubsystem;

/**
 * Pure graph -> spell-definition compiler boundary.
 *
 * USpellGraphSubsystem owns graph storage and structural validation.
 * FSpellGraphCompiler owns interpretation of a valid semantic graph.
 *
 * This class does not mutate SpellCreation, UI, input, preview or execution.
 */
struct SPELLGRAPH_API FSpellGraphCompiler
{
    static FSpellGraphCompileResult Compile(
        const TArray<FSpellGraphNode>& Nodes,
        const UWorldCodexSubsystem& Codex);
};
