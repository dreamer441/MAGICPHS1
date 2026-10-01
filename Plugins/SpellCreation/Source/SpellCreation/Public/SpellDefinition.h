#pragma once

#include "CoreMinimal.h"
#include "MaterialPhysicalProperties.h"
#include "PhysicalBodyState.h"
#include "SpellSpatialTypes.h"
#include "SpellPatternTypes.h"
#include "SpellMotionTypes.h"
#include "SpellDefinition.generated.h"

struct FEarthSpellDefinition;

UENUM(BlueprintType)
enum class ESpellElement : uint8 { Earth };

UENUM(BlueprintType)
enum class ESpellShape : uint8 { Sphere, Cube, Cone };

USTRUCT(BlueprintType)
struct SPELLCREATION_API FSpellShapeDefinition
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spell|Shape") float SphereRadiusCm = 22.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spell|Shape") float CubeXcm = 100.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spell|Shape") float CubeYcm = 100.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spell|Shape") float CubeZcm = 100.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spell|Shape") float ConeRadiusCm = 40.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spell|Shape") float ConeHeightCm = 120.0f;
};

USTRUCT(BlueprintType)
struct SPELLCREATION_API FSpellDefinition
{
    GENERATED_BODY()
    FSpellDefinition() { Material.Toughness = 0.40f; }
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spell") ESpellElement Element = ESpellElement::Earth;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spell") ESpellShape Shape = ESpellShape::Sphere;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spell") FSpellShapeDefinition ShapeDefinition;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spell") FMaterialPhysicalProperties Material;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spell") float DistanceM = 3.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spell|Spatial") ESpellOrientationAxis Orientation = ESpellOrientationAxis::Up;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spell|Pattern") FSpellPatternDefinition Pattern;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spell|Motion") ESpellMotionDirection MotionDirection = ESpellMotionDirection::Forward;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spell") float SpeedMps = 30.0f;
};

USTRUCT(BlueprintType)
struct SPELLCREATION_API FResolvedSpell
{
    GENERATED_BODY()
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Spell") FSpellDefinition Definition;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Spell") FPhysicalBodyState Body;
};

struct SPELLCREATION_API FSpellDefinitionAdapter
{
    static FSpellDefinition FromLegacyEarth(const FEarthSpellDefinition& Legacy);
    static FEarthSpellDefinition ToLegacyEarth(const FSpellDefinition& Definition);
    static FResolvedSpell Resolve(const FSpellDefinition& Definition);
};
