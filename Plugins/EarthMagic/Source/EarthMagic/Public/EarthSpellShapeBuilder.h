#pragma once

#include "CoreMinimal.h"
#include "SpellDefinition.h"

class UStaticMesh;

/**
 * Runtime shape-realization node for Earth spell bodies.
 * For the prototype it maps the construction definition to stable engine basic meshes.
 * Callers do not depend on how the shape is rendered/collided, so this can later be
 * replaced by procedural/generated geometry without touching SpellCreation or ImpactSystem.
 */
struct EARTHMAGIC_API FEarthSpellShapeBuilder
{
    static UStaticMesh* ResolveStaticMesh(ESpellShape Shape);
    static FVector CalculateMeshScale(const FSpellDefinition& Spell);
};
