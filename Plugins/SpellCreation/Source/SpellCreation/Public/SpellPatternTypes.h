#pragma once

#include "CoreMinimal.h"
#include "SpellPatternTypes.generated.h"

UENUM(BlueprintType)
enum class ESpellArrangement : uint8
{
    Line   UMETA(DisplayName="Line"),
    Circle UMETA(DisplayName="Circle")
};

UENUM(BlueprintType)
enum class ESpellPatternAxis : uint8
{
    Forward UMETA(DisplayName="Forward"),
    Right   UMETA(DisplayName="Right"),
    Up      UMETA(DisplayName="Up")
};

UENUM(BlueprintType)
enum class ESpellPatternOrientation : uint8
{
    Shared  UMETA(DisplayName="Shared"),
    Outward UMETA(DisplayName="Outward"),
    Inward  UMETA(DisplayName="Inward"),
    Tangent UMETA(DisplayName="Tangent")
};

USTRUCT(BlueprintType)
struct SPELLCREATION_API FSpellPatternDefinition
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spell|Pattern")
    int32 Amount = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spell|Pattern")
    ESpellArrangement Arrangement = ESpellArrangement::Line;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spell|Pattern")
    ESpellPatternAxis LineAxis = ESpellPatternAxis::Right;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spell|Pattern")
    ESpellPatternOrientation InstanceOrientation = ESpellPatternOrientation::Shared;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spell|Pattern", meta=(Units="cm"))
    float SpacingCm = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spell|Pattern", meta=(Units="cm"))
    float CircleRadiusCm = 220.0f;
};

namespace SpellPatternRanges
{
    constexpr int32 MinAmount = 1;
    constexpr int32 MaxAmount = 20;
    constexpr float MinSpacingCm = 25.0f;
    constexpr float MaxSpacingCm = 500.0f;
    constexpr float MinCircleRadiusCm = 50.0f;
    constexpr float MaxCircleRadiusCm = 600.0f;
}
