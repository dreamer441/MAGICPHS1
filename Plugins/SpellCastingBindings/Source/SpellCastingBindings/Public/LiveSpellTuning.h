#pragma once

#include "CoreMinimal.h"

/** Hold-time feel only. Physical/editor ranges live in SpellCreation/SpellParameterRanges.h. */
namespace LiveSpellTuning
{
    constexpr float FullChargeSeconds = 3.0f;

    /** Quadratic ramp: 1 s ~= 11%, 2 s ~= 44%, 3 s = 100%. */
    inline float ChargeToValue01(const float HeldSeconds)
    {
        const float Linear = FMath::Clamp(HeldSeconds / FullChargeSeconds, 0.0f, 1.0f);
        return Linear * Linear;
    }
}
