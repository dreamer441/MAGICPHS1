#include "PhysicalBodyMath.h"

float UPhysicalBodyMath::MassFromDensityAndVolume(float DensityKgPerM3, float VolumeM3)
{
    return FMath::Max(DensityKgPerM3, 0.0f) * FMath::Max(VolumeM3, 0.0f);
}

float UPhysicalBodyMath::DensityFromMassAndVolume(float MassKg, float VolumeM3)
{
    return FMath::Max(MassKg, 0.0f) / FMath::Max(VolumeM3, 0.000001f);
}
