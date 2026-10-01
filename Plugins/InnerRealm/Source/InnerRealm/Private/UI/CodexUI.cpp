#include "InnerRealmSubsystem.h"

#include "WorldCodexSubsystem.h"
#include "WorldCodexTypes.h"

FText UInnerRealmSubsystem::GetSelectedCodexTitle() const
{
    const UWorldCodexSubsystem* Codex = GetWorldCodex();
    const FCodexEntry* Entry = Codex ? Codex->FindEntry(SelectedCodexConcept) : nullptr;
    return Entry ? Entry->DisplayName : FText::FromString(TEXT("CODEX"));
}

FText UInnerRealmSubsystem::GetSelectedCodexSign() const
{
    const UWorldCodexSubsystem* Codex = GetWorldCodex();
    const FCodexEntry* Entry = Codex ? Codex->FindEntry(SelectedCodexConcept) : nullptr;
    return Entry
        ? FText::FromString(FString::Printf(TEXT("SIGN  [ %s ]"), *Entry->Sign.Glyph))
        : FText::GetEmpty();
}

FText UInnerRealmSubsystem::GetSelectedCodexDetails() const
{
    const UWorldCodexSubsystem* Codex = GetWorldCodex();
    const FCodexEntry* Entry = Codex ? Codex->FindEntry(SelectedCodexConcept) : nullptr;
    if (!Entry)
    {
        return FText::FromString(TEXT("Select a Codex entry."));
    }

    auto JoinNames = [](const TArray<FName>& Names) -> FString
    {
        if (Names.Num() == 0) return TEXT("-");
        TArray<FString> Parts;
        Parts.Reserve(Names.Num());
        for (const FName Name : Names) Parts.Add(Name.ToString());
        return FString::Join(Parts, TEXT(", "));
    };

    TArray<FString> RelationLines;
    for (const FCodexRelation& Relation : Entry->Relationships)
    {
        RelationLines.Add(FString::Printf(
            TEXT("%s -> %s"),
            *Relation.Relation.ToString(),
            *Relation.TargetConceptId.ToString()));
    }

    const FString Relations = RelationLines.Num() > 0
        ? FString::Join(RelationLines, TEXT("\n"))
        : TEXT("-");

    FString Details = FString::Printf(
        TEXT("CONCEPT ID\n%s\n\nCATEGORY\n%s\n\nTIER\n%s\n\nSTATUS\n%s\n\nOWNING SYSTEM\n%s\n\nDESCRIPTION\n%s\n\nPROVIDES CAPABILITIES\n%s\n\nREQUIRES CAPABILITIES\n%s\n\nRELATIONSHIPS\n%s"),
        *Entry->ConceptId.ToString(),
        *CodexCategoryToText(Entry->Category).ToString(),
        *CodexTierToText(Entry->Tier).ToString(),
        *CodexImplementationStateToText(Entry->ImplementationState).ToString(),
        *Entry->OwningSystem.ToString(),
        *Entry->Description.ToString(),
        *JoinNames(Entry->ProvidesCapabilities),
        *JoinNames(Entry->RequiresCapabilities),
        *Relations);

    if (!Entry->DeveloperNotes.IsEmpty())
    {
        Details += FString::Printf(TEXT("\n\nDEVELOPER NOTE\n%s"), *Entry->DeveloperNotes.ToString());
    }

    return FText::FromString(Details);
}
