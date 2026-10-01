#pragma once

#include "CoreMinimal.h"

namespace SpellParameterRanges
{
    constexpr float MinSphereRadiusCm = 10.0f, MaxSphereRadiusCm = 300.0f;
    constexpr float MinCubeSideCm = 20.0f, MaxCubeSideCm = 600.0f;
    constexpr float MinConeRadiusCm = 10.0f, MaxConeRadiusCm = 300.0f;
    constexpr float MinConeHeightCm = 30.0f, MaxConeHeightCm = 800.0f;
    constexpr float MinSpeedMps = 0.0f, MaxSpeedMps = 180.0f;
    constexpr float MinDensityKgPerM3 = 500.0f, MaxDensityKgPerM3 = 4500.0f;
    constexpr float MinDistanceM = 0.0f, MaxDistanceM = 25.0f;
    inline float Normalize(float Value, float Min, float Max) { return FMath::Clamp((Value - Min) / FMath::Max(Max - Min, KINDA_SMALL_NUMBER), 0.0f, 1.0f); }
    inline float Denormalize(float Value, float Min, float Max) { return FMath::Lerp(Min, Max, FMath::Clamp(Value, 0.0f, 1.0f)); }
}
