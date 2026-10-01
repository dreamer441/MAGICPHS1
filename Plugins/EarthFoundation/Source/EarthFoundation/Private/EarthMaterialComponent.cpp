#include "EarthMaterialComponent.h"
UEarthMaterialComponent::UEarthMaterialComponent() { PrimaryComponentTick.bCanEverTick = false; }
FEarthPhysicalProperties UEarthMaterialComponent::GetProperties() const { return Definition ? Definition->Properties : InlineProperties; }
FMaterialPhysicalProperties UEarthMaterialComponent::GetMaterialState() const { return GetProperties().Material; }
