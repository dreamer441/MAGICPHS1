#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "WorldCodexTypes.h"
#include "WorldCodexSubsystem.generated.h"

/**
 * Canonical dictionary of world concepts.
 *
 * It documents meaning/sign/tier/capability relationships only. It does not
 * execute spells, own physics, render runes, or interpret keyboard input.
 */
UCLASS()
class WORLDCODEX_API UWorldCodexSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    void RegisterEntry(const FCodexEntry& Entry);

    const TArray<FCodexEntry>& GetAllEntries() const { return Entries; }
    const FCodexEntry* FindEntry(FName ConceptId) const;
    TArray<const FCodexEntry*> GetEntriesByCategory(ECodexCategory Category) const;

    bool RequirementsSatisfied(
        const FCodexEntry& Entry,
        const TSet<FName>& AvailableCapabilities,
        TArray<FName>* OutMissingCapabilities = nullptr) const;

    void AddProvidedCapabilities(
        const FCodexEntry& Entry,
        TSet<FName>& InOutCapabilities) const;

    /** Tier-only structural grammar check. Semantic compatibility is separate. */
    bool TierAllowsConnection(
        const FCodexEntry& Parent,
        const FCodexEntry& Child) const;

private:
    UPROPERTY()
    TArray<FCodexEntry> Entries;

    TMap<FName, int32> EntryIndexById;

    void RegisterBuiltInVocabulary();
};
