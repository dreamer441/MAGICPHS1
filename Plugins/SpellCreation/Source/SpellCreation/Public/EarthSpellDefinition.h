#pragma once

#include "CoreMinimal.h"
#include "SpellSpatialTypes.h"
#include "SpellPatternTypes.h"
#include "SpellMotionTypes.h"
#include "EarthSpellDefinition.generated.h"

/** Deprecated Earth shape contract retained for reflected compatibility. */
UENUM(BlueprintType)
enum class EEarthSpellShape : uint8
{
    Sphere UMETA(DisplayName="Sphere"),
    Cube   UMETA(DisplayName="Cube"),
    Cone   UMETA(DisplayName="Cone")
};

/**
 * Deprecated Earth spell construction data retained for Blueprint compatibility.
 *
 * Density is an authored material property. Mass is a derived cached body
 * value (density * volume) maintained by SpellCreation before execution.
 */
USTRUCT(BlueprintType)
struct SPELLCREATION_API FEarthSpellDefinition
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spell|Shape")
    EEarthSpellShape Shape = EEarthSpellShape::Sphere;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spell|Shape", meta=(ClampMin="1.0", Units="cm"))
    float SphereRadiusCm = 22.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spell|Shape", meta=(ClampMin="1.0", Units="cm"))
    float CubeXcm = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spell|Shape", meta=(ClampMin="1.0", Units="cm"))
    float CubeYcm = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spell|Shape", meta=(ClampMin="1.0", Units="cm"))
    float CubeZcm = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spell|Shape", meta=(ClampMin="1.0", Units="cm"))
    float ConeRadiusCm = 40.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spell|Shape", meta=(ClampMin="1.0", Units="cm"))
    float ConeHeightCm = 120.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spell|Spatial", meta=(ClampMin="0.0", Units="m"))
    float DistanceM = 3.0f;

    /** Axis the constructed shape is aligned to inside the cast frame. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spell|Spatial")
    ESpellOrientationAxis Orientation = ESpellOrientationAxis::Up;

    // Legacy shadow of generic Pattern fields. This keeps the current Workbench
    // adapter lossless until the UI edits FSpellDefinition directly.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spell|Pattern")
    int32 Amount = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spell|Pattern")
    ESpellArrangement Arrangement = ESpellArrangement::Line;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spell|Pattern")
    ESpellPatternAxis PatternAxis = ESpellPatternAxis::Right;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spell|Pattern")
    ESpellPatternOrientation PatternOrientation = ESpellPatternOrientation::Shared;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spell|Pattern", meta=(Units="cm"))
    float SpacingCm = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spell|Pattern", meta=(Units="cm"))
    float CircleRadiusCm = 220.0f;

    /** Independent movement direction. Orientation controls facing, not travel. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spell|Motion")
    ESpellMotionDirection MotionDirection = ESpellMotionDirection::Forward;

    /** 0 = create with no initial launch velocity. Gravity still applies. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spell|Motion", meta=(ClampMin="0.0"))
    float SpeedMps = 30.0f;

    /** Earth material density. Default soil matches EarthFoundation. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spell|Earth", meta=(ClampMin="1.0"))
    float DensityKgPerM3 = 1600.0f;

    /** Derived cache used by the physical body. SpellCreation keeps it consistent. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spell|Body|Derived", meta=(ClampMin="0.01", Units="kg"))
    float MassKg = 71.36f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spell|Earth", meta=(ClampMin="0.0", ClampMax="1.0"))
    float Hardness = 0.25f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spell|Earth", meta=(ClampMin="0.0", ClampMax="1.0"))
    float Toughness = 0.40f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spell|Earth", meta=(ClampMin="0.0", ClampMax="1.0"))
    float Elasticity = 0.08f;

    // Internal Earth defaults. Not exposed in the first live-casting menu.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spell|Earth|Advanced", meta=(ClampMin="0.0", ClampMax="1.0"))
    float Cohesion = 0.35f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spell|Earth|Advanced", meta=(ClampMin="0.0", ClampMax="1.0"))
    float Rigidity = 0.55f;
};
