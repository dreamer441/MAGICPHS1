#include "SpellLoadoutSubsystem.h"

void USpellLoadoutSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    Slots.SetNum(SlotCount);
    EquippedSlotIndex = INDEX_NONE;
}

bool USpellLoadoutSubsystem::IsValidSlot(const int32 SlotIndex) const
{
    return SlotIndex >= 0 && SlotIndex < SlotCount;
}

bool USpellLoadoutSubsystem::IsSlotOccupied(const int32 SlotIndex) const
{
    return IsValidSlot(SlotIndex)
        && Slots.IsValidIndex(SlotIndex)
        && Slots[SlotIndex].bOccupied;
}

void USpellLoadoutSubsystem::SaveSlot(
    const int32 SlotIndex,
    const FSpellDefinition& Definition)
{
    if (!IsValidSlot(SlotIndex))
    {
        return;
    }

    if (!Slots.IsValidIndex(SlotIndex))
    {
        Slots.SetNum(SlotCount);
    }

    Slots[SlotIndex].bOccupied = true;
    Slots[SlotIndex].Definition = FSpellDefinitionAdapter::Resolve(Definition).Definition;
    EquippedSlotIndex = SlotIndex;
}

bool USpellLoadoutSubsystem::GetSlotSpell(
    const int32 SlotIndex,
    FSpellDefinition& OutDefinition) const
{
    if (!IsSlotOccupied(SlotIndex))
    {
        return false;
    }

    OutDefinition = Slots[SlotIndex].Definition;
    return true;
}

bool USpellLoadoutSubsystem::EquipSlot(const int32 SlotIndex)
{
    if (!IsSlotOccupied(SlotIndex))
    {
        return false;
    }

    EquippedSlotIndex = SlotIndex;
    return true;
}
