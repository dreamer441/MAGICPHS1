#include "SpellModifierContract.h"

const TArray<FSpellModifierContract>& FSpellModifierContracts::GetAll()
{
    static const TArray<FSpellModifierContract> Contracts =
    {
        { ESpellModifierId::Element,            TEXT("Element"),              ESpellModifierValueKind::Choice },
        { ESpellModifierId::Shape,              TEXT("Shape"),                ESpellModifierValueKind::Choice },

        { ESpellModifierId::SphereRadius,       TEXT("Sphere Radius"),        ESpellModifierValueKind::Continuous },
        { ESpellModifierId::CubeX,              TEXT("Cube X"),               ESpellModifierValueKind::Continuous },
        { ESpellModifierId::CubeY,              TEXT("Cube Y"),               ESpellModifierValueKind::Continuous },
        { ESpellModifierId::CubeZ,              TEXT("Cube Z"),               ESpellModifierValueKind::Continuous },
        { ESpellModifierId::ConeRadius,         TEXT("Cone Radius"),          ESpellModifierValueKind::Continuous },
        { ESpellModifierId::ConeHeight,         TEXT("Cone Height"),          ESpellModifierValueKind::Continuous },

        { ESpellModifierId::Speed,              TEXT("Speed"),                ESpellModifierValueKind::Continuous },
        { ESpellModifierId::Direction,          TEXT("Direction"),            ESpellModifierValueKind::Choice },
        { ESpellModifierId::Density,            TEXT("Density"),              ESpellModifierValueKind::Continuous },
        { ESpellModifierId::Hardness,           TEXT("Hardness"),             ESpellModifierValueKind::Continuous },
        { ESpellModifierId::Toughness,          TEXT("Toughness"),            ESpellModifierValueKind::Continuous },
        { ESpellModifierId::Elasticity,         TEXT("Elasticity"),           ESpellModifierValueKind::Continuous },
        { ESpellModifierId::Distance,           TEXT("Distance"),             ESpellModifierValueKind::Continuous },

        { ESpellModifierId::Orientation,        TEXT("Orientation"),          ESpellModifierValueKind::Choice },

        { ESpellModifierId::Amount,             TEXT("Amount"),               ESpellModifierValueKind::Integer },
        { ESpellModifierId::Arrangement,        TEXT("Arrangement"),          ESpellModifierValueKind::Choice },
        { ESpellModifierId::PatternAxis,        TEXT("Line Axis"),            ESpellModifierValueKind::Choice },
        { ESpellModifierId::PatternOrientation, TEXT("Instance Orientation"), ESpellModifierValueKind::Choice },
        { ESpellModifierId::PatternSpacing,     TEXT("Spacing"),              ESpellModifierValueKind::Continuous },
        { ESpellModifierId::CircleRadius,       TEXT("Circle Radius"),        ESpellModifierValueKind::Continuous }
    };

    return Contracts;
}

const FSpellModifierContract* FSpellModifierContracts::Find(const ESpellModifierId Id)
{
    for (const FSpellModifierContract& Contract : GetAll())
    {
        if (Contract.Id == Id)
        {
            return &Contract;
        }
    }
    return nullptr;
}
