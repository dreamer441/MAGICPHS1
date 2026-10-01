#include "SpellGraphSubsystem.h"
#include "SpellGraphCompiler.h"

#include "SpellCreationSubsystem.h"
#include "SpellMotionTypes.h"
#include "SpellParameterRanges.h"
#include "SpellPatternTypes.h"
#include "SpellSpatialTypes.h"
#include "WorldCodexSubsystem.h"
#include "WorldCodexTypes.h"

namespace
{
    bool HasCapability(const FCodexEntry& Entry, const FName Capability)
    {
        return Entry.ProvidesCapabilities.Contains(Capability);
    }
    bool IsMagnitudeParent(const FName ConceptId)
    {
        return
            ConceptId == FName(TEXT("shape.sphere.radius")) ||
            ConceptId == FName(TEXT("shape.cube.x")) ||
            ConceptId == FName(TEXT("shape.cube.y")) ||
            ConceptId == FName(TEXT("shape.cube.z")) ||
            ConceptId == FName(TEXT("shape.cone.radius")) ||
            ConceptId == FName(TEXT("shape.cone.height")) ||
            ConceptId == FName(TEXT("material.density")) ||
            ConceptId == FName(TEXT("material.hardness")) ||
            ConceptId == FName(TEXT("material.toughness")) ||
            ConceptId == FName(TEXT("material.elasticity")) ||
            ConceptId == FName(TEXT("spatial.distance")) ||
            ConceptId == FName(TEXT("pattern.amount")) ||
            ConceptId == FName(TEXT("pattern.spacing")) ||
            ConceptId == FName(TEXT("pattern.circle_radius")) ||
            ConceptId == FName(TEXT("motion.speed"));
    }

    bool IsExclusiveConcept(const FName ConceptId)
    {
        return
            ConceptId == FName(TEXT("motion.direction")) ||
            ConceptId == FName(TEXT("spatial.orientation")) ||
            ConceptId == FName(TEXT("pattern.orientation")) ||
            ConceptId == FName(TEXT("pattern.line_axis"));
    }

    bool IsPatternArrangement(const FName ConceptId)
    {
        return
            ConceptId == FName(TEXT("pattern.line")) ||
            ConceptId == FName(TEXT("pattern.circle"));
    }
}

void USpellGraphSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    Nodes.Reset();
    NextOrder = 1;

    LastCompileResult = FSpellGraphCompileResult();
    LastCompileResult.Message = FText::FromString(
        TEXT("Canvas empty. Add the Tier I Earth sign to begin."));
}

UWorldCodexSubsystem* USpellGraphSubsystem::GetCodex() const
{
    UWorld* World = GetWorld();
    return World ? World->GetSubsystem<UWorldCodexSubsystem>() : nullptr;
}

const FSpellGraphNode* USpellGraphSubsystem::FindNode(const FGuid& NodeId) const
{
    if (!NodeId.IsValid())
    {
        return nullptr;
    }

    for (const FSpellGraphNode& Node : Nodes)
    {
        if (Node.NodeId == NodeId)
        {
            return &Node;
        }
    }

    return nullptr;
}

int32 USpellGraphSubsystem::GetNodeDepth(const FGuid& NodeId) const
{
    int32 Depth = 0;
    const FSpellGraphNode* Current = FindNode(NodeId);
    TSet<FGuid> Visited;

    while (Current && Current->ParentNodeId.IsValid())
    {
        if (Visited.Contains(Current->NodeId))
        {
            break;
        }

        Visited.Add(Current->NodeId);
        ++Depth;
        Current = FindNode(Current->ParentNodeId);
    }

    return Depth;
}

bool USpellGraphSubsystem::IsConceptSupported(const FName ConceptId) const
{
    // V1 compiler vocabulary. Codex may contain many more future concepts.
    return
        ConceptId == FName(TEXT("element.earth")) ||

        ConceptId == FName(TEXT("shape.sphere")) ||
        ConceptId == FName(TEXT("shape.cube")) ||
        ConceptId == FName(TEXT("shape.cone")) ||

        ConceptId == FName(TEXT("shape.sphere.radius")) ||
        ConceptId == FName(TEXT("shape.cube.x")) ||
        ConceptId == FName(TEXT("shape.cube.y")) ||
        ConceptId == FName(TEXT("shape.cube.z")) ||
        ConceptId == FName(TEXT("shape.cone.radius")) ||
        ConceptId == FName(TEXT("shape.cone.height")) ||

        ConceptId == FName(TEXT("material.density")) ||
        ConceptId == FName(TEXT("material.hardness")) ||
        ConceptId == FName(TEXT("material.toughness")) ||
        ConceptId == FName(TEXT("material.elasticity")) ||

        ConceptId == FName(TEXT("spatial.distance")) ||
        ConceptId == FName(TEXT("spatial.orientation")) ||

        ConceptId == FName(TEXT("pattern.amount")) ||
        ConceptId == FName(TEXT("pattern.line")) ||
        ConceptId == FName(TEXT("pattern.circle")) ||
        ConceptId == FName(TEXT("pattern.line_axis")) ||
        ConceptId == FName(TEXT("pattern.spacing")) ||
        ConceptId == FName(TEXT("pattern.circle_radius")) ||
        ConceptId == FName(TEXT("pattern.orientation")) ||

        ConceptId == FName(TEXT("motion.speed")) ||
        ConceptId == FName(TEXT("motion.direction")) ||

        ConceptId == FName(TEXT("value.magnitude.0")) ||
        ConceptId == FName(TEXT("value.magnitude.1")) ||
        ConceptId == FName(TEXT("value.magnitude.2")) ||
        ConceptId == FName(TEXT("value.magnitude.3")) ||
        ConceptId == FName(TEXT("value.magnitude.4")) ||
        ConceptId == FName(TEXT("value.magnitude.5")) ||

        ConceptId == FName(TEXT("direction.forward")) ||
        ConceptId == FName(TEXT("direction.backward")) ||
        ConceptId == FName(TEXT("direction.up")) ||
        ConceptId == FName(TEXT("direction.down")) ||
        ConceptId == FName(TEXT("direction.outward")) ||
        ConceptId == FName(TEXT("direction.inward")) ||
        ConceptId == FName(TEXT("direction.tangent")) ||

        ConceptId == FName(TEXT("orientation.forward")) ||
        ConceptId == FName(TEXT("orientation.right")) ||
        ConceptId == FName(TEXT("orientation.up")) ||

        ConceptId == FName(TEXT("axis.forward")) ||
        ConceptId == FName(TEXT("axis.right")) ||
        ConceptId == FName(TEXT("axis.up")) ||

        ConceptId == FName(TEXT("pattern.orientation.shared")) ||
        ConceptId == FName(TEXT("pattern.orientation.outward")) ||
        ConceptId == FName(TEXT("pattern.orientation.inward")) ||
        ConceptId == FName(TEXT("pattern.orientation.tangent"));
}

int32 USpellGraphSubsystem::CountRoots() const
{
    int32 Count = 0;
    for (const FSpellGraphNode& Node : Nodes)
    {
        if (!Node.ParentNodeId.IsValid())
        {
            ++Count;
        }
    }
    return Count;
}

void USpellGraphSubsystem::GatherBranchCapabilities(
    const FGuid& StartNodeId,
    TSet<FName>& OutCapabilities) const
{
    OutCapabilities.Reset();

    const UWorldCodexSubsystem* Codex = GetCodex();
    if (!Codex)
    {
        return;
    }

    const FSpellGraphNode* Current = FindNode(StartNodeId);
    TSet<FGuid> Visited;

    while (Current)
    {
        if (Visited.Contains(Current->NodeId))
        {
            break;
        }

        Visited.Add(Current->NodeId);

        if (const FCodexEntry* Entry = Codex->FindEntry(Current->ConceptId))
        {
            Codex->AddProvidedCapabilities(*Entry, OutCapabilities);
        }

        if (!Current->ParentNodeId.IsValid())
        {
            break;
        }

        Current = FindNode(Current->ParentNodeId);
    }
}

bool USpellGraphSubsystem::IsTerminalCompatible(
    const FName ParentConceptId,
    const FCodexEntry& ChildEntry) const
{
    if (HasCapability(ChildEntry, FName(TEXT("value.magnitude"))))
    {
        return IsMagnitudeParent(ParentConceptId);
    }

    if (HasCapability(ChildEntry, FName(TEXT("value.direction"))))
    {
        return ParentConceptId == FName(TEXT("motion.direction"));
    }

    const FString ChildId = ChildEntry.ConceptId.ToString();

    if (ChildId.StartsWith(TEXT("orientation.")))
    {
        return ParentConceptId == FName(TEXT("spatial.orientation"));
    }

    if (ChildId.StartsWith(TEXT("axis.")))
    {
        return ParentConceptId == FName(TEXT("pattern.line_axis"));
    }

    if (ChildId.StartsWith(TEXT("pattern.orientation.")))
    {
        return ParentConceptId == FName(TEXT("pattern.orientation"));
    }

    return false;
}

bool USpellGraphSubsystem::ValidateAddition(
    const FCodexEntry& Entry,
    const FGuid& ParentNodeId,
    FText& OutReason) const
{
    const UWorldCodexSubsystem* Codex = GetCodex();
    if (!Codex)
    {
        OutReason = FText::FromString(TEXT("World Codex unavailable."));
        return false;
    }

    if (!IsConceptSupported(Entry.ConceptId))
    {
        OutReason = FText::FromString(
            TEXT("This Sign is in the Codex, but the Rune Canvas V1 compiler does not execute it yet."));
        return false;
    }

    if (Entry.Tier == ECodexTier::TierI)
    {
        if (CountRoots() > 0)
        {
            OutReason = FText::FromString(
                TEXT("Rune Canvas V1 supports one Tier-I construction root. Stacking/multiple roots comes next."));
            return false;
        }

        OutReason = FText::GetEmpty();
        return true;
    }

    const FSpellGraphNode* ParentNode = FindNode(ParentNodeId);
    if (!ParentNode)
    {
        OutReason = FText::FromString(
            TEXT("Select an existing graph node first, then choose the Sign to attach."));
        return false;
    }

    const FCodexEntry* ParentEntry = Codex->FindEntry(ParentNode->ConceptId);
    if (!ParentEntry)
    {
        OutReason = FText::FromString(TEXT("Selected parent has no Codex entry."));
        return false;
    }

    if (!Codex->TierAllowsConnection(*ParentEntry, Entry))
    {
        OutReason = FText::FromString(
            TEXT("Tier grammar rejects this connection. Foundations cannot have parents; terminals cannot have children."));
        return false;
    }

    if (Entry.Tier == ECodexTier::TierIII &&
        !IsTerminalCompatible(ParentNode->ConceptId, Entry))
    {
        OutReason = FText::FromString(
            TEXT("That terminal value does not belong to the selected Tier-II concept."));
        return false;
    }

    TSet<FName> BranchCapabilities;
    GatherBranchCapabilities(ParentNodeId, BranchCapabilities);

    TArray<FName> Missing;
    if (!Codex->RequirementsSatisfied(Entry, BranchCapabilities, &Missing))
    {
        TArray<FString> Parts;
        for (const FName Capability : Missing)
        {
            Parts.Add(Capability.ToString());
        }

        OutReason = FText::FromString(FString::Printf(
            TEXT("Connection is missing capability: %s"),
            *FString::Join(Parts, TEXT(", "))));
        return false;
    }

    // One occurrence of ordinary operator concepts in V1.
    if (Entry.Tier == ECodexTier::TierII)
    {
        for (const FSpellGraphNode& Node : Nodes)
        {
            if (Node.ConceptId == Entry.ConceptId)
            {
                OutReason = FText::FromString(
                    TEXT("That operator already exists in this V1 construction."));
                return false;
            }
        }

        if (Entry.Category == ECodexCategory::Shape)
        {
            for (const FSpellGraphNode& Node : Nodes)
            {
                const FCodexEntry* Existing = Codex->FindEntry(Node.ConceptId);
                if (Existing && Existing->Category == ECodexCategory::Shape)
                {
                    OutReason = FText::FromString(
                        TEXT("A V1 construction can contain one Shape. Remove the existing Shape to replace it."));
                    return false;
                }
            }
        }

        if (IsPatternArrangement(Entry.ConceptId))
        {
            for (const FSpellGraphNode& Node : Nodes)
            {
                if (IsPatternArrangement(Node.ConceptId))
                {
                    OutReason = FText::FromString(
                        TEXT("A V1 construction can contain one Pattern arrangement."));
                    return false;
                }
            }
        }

        if (IsExclusiveConcept(Entry.ConceptId))
        {
            for (const FSpellGraphNode& Node : Nodes)
            {
                if (Node.ConceptId == Entry.ConceptId)
                {
                    OutReason = FText::FromString(TEXT("That operator already exists."));
                    return false;
                }
            }
        }
    }

    OutReason = FText::GetEmpty();
    return true;
}

FGuid USpellGraphSubsystem::AddConcept(
    const FName ConceptId,
    const FGuid& ParentNodeId)
{
    UWorldCodexSubsystem* Codex = GetCodex();
    const FCodexEntry* Entry = Codex ? Codex->FindEntry(ConceptId) : nullptr;

    if (!Entry)
    {
        LastCompileResult.bSuccess = false;
        LastCompileResult.Message = FText::FromString(TEXT("Unknown Codex concept."));
        return FGuid();
    }

    const FGuid EffectiveParent =
        Entry->Tier == ECodexTier::TierI ? FGuid() : ParentNodeId;

    FText Reason;
    if (!ValidateAddition(*Entry, EffectiveParent, Reason))
    {
        LastCompileResult.bSuccess = false;
        LastCompileResult.Message = Reason;
        return FGuid();
    }

    // A Tier-II concept has one terminal value in V1. Selecting another value
    // replaces the existing terminal rather than forcing manual deletion.
    if (Entry->Tier == ECodexTier::TierIII)
    {
        TArray<FGuid> ExistingTerminalChildren;

        for (const FSpellGraphNode& Node : Nodes)
        {
            if (Node.ParentNodeId != EffectiveParent)
            {
                continue;
            }

            const FCodexEntry* ExistingEntry = Codex->FindEntry(Node.ConceptId);
            if (ExistingEntry && ExistingEntry->Tier == ECodexTier::TierIII)
            {
                ExistingTerminalChildren.Add(Node.NodeId);
            }
        }

        for (const FGuid& ExistingId : ExistingTerminalChildren)
        {
            RemoveSubtree(ExistingId);
        }
    }

    FSpellGraphNode Node;
    Node.NodeId = FGuid::NewGuid();
    Node.ConceptId = ConceptId;
    Node.ParentNodeId = EffectiveParent;
    Node.Order = NextOrder++;
    Nodes.Add(Node);

    CompileAndApply();
    return Node.NodeId;
}

void USpellGraphSubsystem::RemoveDescendantsRecursive(
    const FGuid& ParentNodeId,
    TSet<FGuid>& InOutIds) const
{
    for (const FSpellGraphNode& Node : Nodes)
    {
        if (Node.ParentNodeId == ParentNodeId &&
            !InOutIds.Contains(Node.NodeId))
        {
            InOutIds.Add(Node.NodeId);
            RemoveDescendantsRecursive(Node.NodeId, InOutIds);
        }
    }
}

bool USpellGraphSubsystem::RemoveSubtree(const FGuid& NodeId)
{
    if (!FindNode(NodeId))
    {
        return false;
    }

    TSet<FGuid> RemoveIds;
    RemoveIds.Add(NodeId);
    RemoveDescendantsRecursive(NodeId, RemoveIds);

    Nodes.RemoveAll([&RemoveIds](const FSpellGraphNode& Node)
    {
        return RemoveIds.Contains(Node.NodeId);
    });

    CompileAndApply();
    return true;
}

void USpellGraphSubsystem::ClearGraph()
{
    Nodes.Reset();
    NextOrder = 1;

    LastCompileResult = FSpellGraphCompileResult();
    LastCompileResult.Message = FText::FromString(
        TEXT("Canvas cleared. Add Earth to start a new construction."));
}

FSpellGraphCompileResult USpellGraphSubsystem::CompileAndApply()
{
    UWorld* World = GetWorld();
    UWorldCodexSubsystem* Codex = GetCodex();
    USpellCreationSubsystem* SpellCreation =
        World ? World->GetSubsystem<USpellCreationSubsystem>() : nullptr;

    if (!World || !Codex || !SpellCreation)
    {
        FSpellGraphCompileResult Result;
        Result.Message = FText::FromString(
            TEXT("Compiler unavailable: missing world, Codex, or SpellCreation."));
        LastCompileResult = Result;
        return Result;
    }

    FSpellGraphCompileResult Result =
        FSpellGraphCompiler::Compile(Nodes, *Codex);

    if (Result.bSuccess)
    {
        SpellCreation->SetStoredGenericSpellDefinition(
            Result.Definition);
    }

    LastCompileResult = Result;
    return Result;
}
