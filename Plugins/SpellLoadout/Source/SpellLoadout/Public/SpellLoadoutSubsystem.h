#pragma once

#include "CoreMinimal.h"
#include "SpellDefinition.h"
#include "Subsystems/WorldSubsystem.h"
#include "SpellLoadoutSubsystem.generated.h"

USTRUCT()
struct SPELLLOADOUT_API FPreparedSpellSlot
{
    GENERATED_BODY()

    UPROPERTY()
    bool bOccupied = false;

    UPROPERTY()
    FSpellDefinition Definition;
};

/**
 * Prepared-spell data node.
 * SpellCreation owns the editable Workbench definition.
 * SpellLoadout owns ten saved snapshots.
 */
UCLASS()
class SPELLLOADOUT_API USpellLoadoutSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    static constexpr int32 SlotCount = 10;

    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    bool IsValidSlot(int32 SlotIndex) const;
    bool IsSlotOccupied(int32 SlotIndex) const;
    void SaveSlot(int32 SlotIndex, const FSpellDefinition& Definition);
    bool GetSlotSpell(int32 SlotIndex, FSpellDefinition& OutDefinition) const;

    bool EquipSlot(int32 SlotIndex);
    int32 GetEquippedSlot() const { return EquippedSlotIndex; }
    void ClearEquippedSlot() { EquippedSlotIndex = INDEX_NONE; }

private:
    UPROPERTY()
    TArray<FPreparedSpellSlot> Slots;

    UPROPERTY()
    int32 EquippedSlotIndex = INDEX_NONE;
};
