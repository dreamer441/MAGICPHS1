#pragma once

#include "CoreMinimal.h"
#include "SpellSpatialTypes.generated.h"

/** Simple cast-frame-relative orientation vocabulary for constructed shapes. */
UENUM(BlueprintType)
enum class ESpellOrientationAxis : uint8
{
    Forward UMETA(DisplayName="Forward / X"),
    Right   UMETA(DisplayName="Right / Y"),
    Up      UMETA(DisplayName="Up / Z")
};
