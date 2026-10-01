#include "EarthSpellShapeBuilder.h"

#include "Engine/StaticMesh.h"
#include "UObject/UObjectGlobals.h"

UStaticMesh* FEarthSpellShapeBuilder::ResolveStaticMesh(const ESpellShape Shape)
{
    // Engine BasicShapes are stable, convex, visible, and ship with collision.
    // Keeping the asset mapping inside this node prevents shape-specific asset knowledge
    // from leaking into the spell definition or impact code.
    switch (Shape)
    {
        case ESpellShape::Sphere:
            return LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
        case ESpellShape::Cube:
            return LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
        case ESpellShape::Cone:
        default:
            return LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cone.Cone"));
    }
}

FVector FEarthSpellShapeBuilder::CalculateMeshScale(const FSpellDefinition& Spell)
{
    // Engine basic sphere/cube/cone are approximately 100 cm in their base dimensions.
    // Sphere radius is 50 cm; cone base diameter is 100 cm and height is 100 cm.
    const FSpellShapeDefinition& Shape = Spell.ShapeDefinition;
    switch (Spell.Shape)
    {
        case ESpellShape::Sphere:
        {
            const float Uniform = FMath::Max(Shape.SphereRadiusCm, 1.0f) / 50.0f;
            return FVector(Uniform);
        }
        case ESpellShape::Cube:
            return FVector(
                FMath::Max(Shape.CubeXcm, 1.0f) / 100.0f,
                FMath::Max(Shape.CubeYcm, 1.0f) / 100.0f,
                FMath::Max(Shape.CubeZcm, 1.0f) / 100.0f);
        case ESpellShape::Cone:
        default:
        {
            const float Radial = FMath::Max(Shape.ConeRadiusCm, 1.0f) / 50.0f;
            const float Height = FMath::Max(Shape.ConeHeightCm, 1.0f) / 100.0f;
            return FVector(Radial, Radial, Height);
        }
    }
}
