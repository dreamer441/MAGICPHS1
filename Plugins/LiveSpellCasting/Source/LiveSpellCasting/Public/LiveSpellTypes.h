#pragma once

#include "CoreMinimal.h"
#include "LiveSpellTypes.generated.h"

UENUM()
enum class ELiveSpellParameter : uint8
{
    SphereRadius, CubeX, CubeY, CubeZ, ConeRadius, ConeHeight,
    Speed, Density, Distance, Hardness, Toughness, Elasticity
};

UENUM()
enum class ELiveSpellStage : uint8
{
    Empty, ElementSelected, ShapeSelected, ModifierActive
};
