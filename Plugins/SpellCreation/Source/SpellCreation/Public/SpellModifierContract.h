#pragma once

#include "CoreMinimal.h"

enum class ESpellModifierId : uint8
{
    Element,
    Shape,

    SphereRadius,
    CubeX,
    CubeY,
    CubeZ,
    ConeRadius,
    ConeHeight,

    Speed,
    Direction,
    Density,
    Hardness,
    Toughness,
    Elasticity,
    Distance,

    Orientation,

    Amount,
    Arrangement,
    PatternAxis,
    PatternOrientation,
    PatternSpacing,
    CircleRadius
};

enum class ESpellModifierValueKind : uint8
{
    Choice,
    Continuous,
    Integer
};

struct SPELLCREATION_API FSpellModifierContract
{
    ESpellModifierId Id;
    const TCHAR* DisplayName;
    ESpellModifierValueKind ValueKind;

    bool bHasPersistentDefault = true;
    bool bSupportsOptionalLiveBinding = true;
};

struct SPELLCREATION_API FSpellModifierContracts
{
    static const TArray<FSpellModifierContract>& GetAll();
    static const FSpellModifierContract* Find(ESpellModifierId Id);
};
