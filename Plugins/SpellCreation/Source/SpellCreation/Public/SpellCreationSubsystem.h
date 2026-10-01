#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "EarthSpellDefinition.h"
#include "SpellDefinition.h"
#include "SpellCreationSubsystem.generated.h"

/**
 * Data node for spell construction.
 * Owns canonical generic spell data and a synchronized legacy reflected shadow.
 */
UCLASS()
class SPELLCREATION_API USpellCreationSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    UFUNCTION(BlueprintPure, Category="Spell Creation")
    FSpellDefinition GetStoredGenericSpellDefinition() const { return StoredGenericSpell; }

    UFUNCTION(BlueprintCallable, Category="Spell Creation")
    void SetStoredGenericSpellDefinition(const FSpellDefinition& NewSpell);

    /**
     * Compatibility projection for older Earth-specific Blueprint/UI callers.
     * New code should use GetStoredGenericSpellDefinition().
     */
    UFUNCTION(BlueprintPure, Category="Spell Creation")
    FEarthSpellDefinition GetStoredSpellDefinition() const
    {
        return FSpellDefinitionAdapter::ToLegacyEarth(StoredGenericSpell);
    }

    UFUNCTION(BlueprintCallable, Category="Spell Creation")
    void SetStoredSpellDefinition(const FEarthSpellDefinition& NewSpell);

    UFUNCTION(BlueprintCallable, Category="Spell Creation")
    void ResetToDefaultEarthSpell();

private:
    /** Canonical single-spell runtime value. */
    UPROPERTY()
    FSpellDefinition StoredGenericSpell;
};
