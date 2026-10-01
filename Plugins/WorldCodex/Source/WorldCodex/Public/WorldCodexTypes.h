#pragma once

#include "CoreMinimal.h"
#include "WorldCodexTypes.generated.h"

UENUM(BlueprintType)
enum class ECodexCategory : uint8
{
    Element,
    Shape,
    ShapeParameter,
    MaterialProperty,
    Spatial,
    Pattern,
    Motion,
    Action,
    WorldObject,
    WorldState,
    Event,
    Logic,
    Value,
    PhysicsConcept
};

/**
 * Structural role in the future Rune Canvas grammar.
 *
 * Tier I   = foundation/root. Cannot be attached beneath another concept.
 * Tier II  = operator/modifier/reference. Can have a parent and can accept children.
 * Tier III = terminal/leaf value. Can attach to something but accepts no children.
 *
 * Tier is only structural grammar. Capability/relationship rules still decide
 * whether a specific otherwise-valid connection is meaningful.
 */
UENUM(BlueprintType)
enum class ECodexTier : uint8
{
    TierI,
    TierII,
    TierIII
};

UENUM(BlueprintType)
enum class ECodexImplementationState : uint8
{
    Implemented,
    CodexOnly,
    Planned
};

USTRUCT(BlueprintType)
struct WORLDCODEX_API FCodexSignDefinition
{
    GENERATED_BODY()

    /** Temporary readable glyph. Later this can point at authored rune art. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Codex|Sign")
    FString Glyph = TEXT("?");

    /** Stable optional visual asset key for future rune rendering. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Codex|Sign")
    FName SignAssetId = NAME_None;
};

USTRUCT(BlueprintType)
struct WORLDCODEX_API FCodexRelation
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Codex|Relation")
    FName Relation = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Codex|Relation")
    FName TargetConceptId = NAME_None;

    FCodexRelation() = default;

    FCodexRelation(const FName InRelation, const FName InTargetConceptId)
        : Relation(InRelation)
        , TargetConceptId(InTargetConceptId)
    {
    }
};

USTRUCT(BlueprintType)
struct WORLDCODEX_API FCodexEntry
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Codex")
    FName ConceptId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Codex")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Codex")
    FText Description;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Codex")
    ECodexCategory Category = ECodexCategory::Value;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Codex")
    ECodexTier Tier = ECodexTier::TierII;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Codex")
    FCodexSignDefinition Sign;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Codex|Capabilities")
    TArray<FName> ProvidesCapabilities;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Codex|Capabilities")
    TArray<FName> RequiresCapabilities;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Codex|Relations")
    TArray<FCodexRelation> Relationships;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Codex|Development")
    ECodexImplementationState ImplementationState = ECodexImplementationState::CodexOnly;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Codex|Development")
    FName OwningSystem = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Codex|Development")
    FText DeveloperNotes;

    bool CanHaveParent() const { return Tier != ECodexTier::TierI; }
    bool CanHaveChildren() const { return Tier != ECodexTier::TierIII; }
};

WORLDCODEX_API FText CodexCategoryToText(ECodexCategory Category);
WORLDCODEX_API FText CodexTierToText(ECodexTier Tier);
WORLDCODEX_API FText CodexImplementationStateToText(ECodexImplementationState State);
