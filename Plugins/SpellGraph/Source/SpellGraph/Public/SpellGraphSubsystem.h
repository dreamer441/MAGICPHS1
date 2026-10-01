#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "SpellGraphTypes.h"
#include "SpellGraphSubsystem.generated.h"

class UWorldCodexSubsystem;
struct FCodexEntry;

/**
 * Rune Canvas semantic state + V1 compiler.
 *
 * V1 intentionally uses click-to-place rather than physical drag/drop:
 * the user selects a graph node, then clicks a Codex Sign to attach it.
 * The graph data is UI-independent, so later drag/drop only changes presentation.
 *
 * V1 supports one Tier-I construction root. The graph representation itself
 * already uses explicit parent IDs, so stacking can extend it without replacing
 * this foundation.
 */
UCLASS()
class SPELLGRAPH_API USpellGraphSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    const TArray<FSpellGraphNode>& GetNodes() const { return Nodes; }
    const FSpellGraphCompileResult& GetLastCompileResult() const { return LastCompileResult; }

    const FSpellGraphNode* FindNode(const FGuid& NodeId) const;
    int32 GetNodeDepth(const FGuid& NodeId) const;

    bool IsConceptSupported(FName ConceptId) const;

    /**
     * Add a concept beneath ParentNodeId.
     * Tier-I roots ignore ParentNodeId and are created at graph root.
     */
    FGuid AddConcept(FName ConceptId, const FGuid& ParentNodeId);

    bool RemoveSubtree(const FGuid& NodeId);
    void ClearGraph();

    FSpellGraphCompileResult CompileAndApply();

private:
    UPROPERTY()
    TArray<FSpellGraphNode> Nodes;

    UPROPERTY()
    FSpellGraphCompileResult LastCompileResult;

    int32 NextOrder = 1;

    UWorldCodexSubsystem* GetCodex() const;

    bool ValidateAddition(
        const FCodexEntry& Entry,
        const FGuid& ParentNodeId,
        FText& OutReason) const;

    void GatherBranchCapabilities(
        const FGuid& StartNodeId,
        TSet<FName>& OutCapabilities) const;

    bool IsTerminalCompatible(
        FName ParentConceptId,
        const FCodexEntry& ChildEntry) const;

    void RemoveDescendantsRecursive(
        const FGuid& ParentNodeId,
        TSet<FGuid>& InOutIds) const;

    int32 CountRoots() const;
};
