#pragma once

#include "CoreMinimal.h"
#include "DynamicMesh/DynamicMesh3.h"
#include "SpellDefinition.h"

/**
 * Geometry-only realization of the editable Earth volume.
 * This class knows about shapes but not impacts, physics, input, or materials.
 * Output coordinates are real Unreal centimeters in the spell body's local frame.
 */
struct EARTHMAGIC_API FEarthSpellDamageGeometry
{
    static UE::Geometry::FDynamicMesh3 Build(const FSpellDefinition& Spell);
};
