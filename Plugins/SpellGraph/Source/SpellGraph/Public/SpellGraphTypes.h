#pragma once

#include "CoreMinimal.h"
#include "SpellDefinition.h"
#include "SpellGraphTypes.generated.h"

/**
 * One semantic Rune Canvas node.
 *
 * Visual layout is deliberately NOT the spell meaning. The permanent meaning is
 * ConceptId + ParentNodeId. A future freeform drag/drop canvas can move nodes
 * without changing compilation semantics.
 */
USTRUCT(BlueprintType)
struct SPELLGRAPH_API FSpellGraphNode
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Spell Graph")
    FGuid NodeId;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Spell Graph")
    FName ConceptId = NAME_None;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Spell Graph")
    FGuid ParentNodeId;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Spell Graph")
    int32 Order = 0;
};

USTRUCT(BlueprintType)
struct SPELLGRAPH_API FSpellGraphCompileResult
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Spell Graph")
    bool bSuccess = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Spell Graph")
    FText Message;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Spell Graph")
    FSpellDefinition Definition;
};
