#pragma once

#include "CoreMinimal.h"
#include "SpellMotionTypes.generated.h"

/**
 * Independent movement direction for a realized spell instance.
 *
 * Orientation answers "which way does the object face?"
 * MotionDirection answers "which way does the object travel?"
 *
 * Outward/Inward/Tangent are pattern-relative. If there is no meaningful
 * pattern-relative vector (for example a single centered object), they fall
 * back to the cast Forward direction.
 */
UENUM(BlueprintType)
enum class ESpellMotionDirection : uint8
{
    Forward  UMETA(DisplayName="Forward"),
    Backward UMETA(DisplayName="Backward"),
    Up       UMETA(DisplayName="Up"),
    Down     UMETA(DisplayName="Down"),
    Outward  UMETA(DisplayName="Outward"),
    Inward   UMETA(DisplayName="Inward"),
    Tangent  UMETA(DisplayName="Tangent")
};
