#include "SpellCreationSubsystem.h"

void USpellCreationSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    ResetToDefaultEarthSpell();
}

void USpellCreationSubsystem::SetStoredGenericSpellDefinition(const FSpellDefinition& NewSpell)
{
    StoredGenericSpell = FSpellDefinitionAdapter::Resolve(NewSpell).Definition;
}

void USpellCreationSubsystem::SetStoredSpellDefinition(const FEarthSpellDefinition& NewSpell)
{
    SetStoredGenericSpellDefinition(FSpellDefinitionAdapter::FromLegacyEarth(NewSpell));
}

void USpellCreationSubsystem::ResetToDefaultEarthSpell()
{
    // The generic default is currently Earth, so reset without going through
    // the legacy Earth compatibility adapter.
    SetStoredGenericSpellDefinition(FSpellDefinition());
}
