#include "EarthSpellDamageGeometry.h"

#include "Generators/GridBoxMeshGenerator.h"
#include "Generators/SphereGenerator.h"
#include "OrientedBoxTypes.h"

using namespace UE::Geometry;

FDynamicMesh3 FEarthSpellDamageGeometry::Build(const FSpellDefinition& Spell)
{
    const FSpellShapeDefinition& Shape = Spell.ShapeDefinition;
    if (Spell.Shape == ESpellShape::Sphere)
    {
        FSphereGenerator Generator;
        Generator.Radius = FMath::Max(Shape.SphereRadiusCm, 1.0f);
        Generator.NumPhi = 24;
        Generator.NumTheta = 24;
        Generator.bPolygroupPerQuad = false;
        Generator.Generate();
        return FDynamicMesh3(&Generator);
    }

    if (Spell.Shape == ESpellShape::Cube)
    {
        FGridBoxMeshGenerator Generator;
        Generator.Box = FOrientedBox3d(
            FVector3d::Zero(),
            FVector3d(
                FMath::Max(Shape.CubeXcm, 1.0f) * 0.5,
                FMath::Max(Shape.CubeYcm, 1.0f) * 0.5,
                FMath::Max(Shape.CubeZcm, 1.0f) * 0.5));
        Generator.EdgeVertices = FIndex3i(9, 9, 9);
        Generator.bPolygroupPerQuad = false;
        Generator.Generate();
        return FDynamicMesh3(&Generator);
    }

    // A closed cone, wound consistently outward. Unlike the original v0.7
    // mesh, its bottom cap exists and is usable by a true 3D mesh difference.
    FDynamicMesh3 Cone;
    const int32 Sides = 32;
    const double Radius = FMath::Max(Shape.ConeRadiusCm, 1.0f);
    const double Height = FMath::Max(Shape.ConeHeightCm, 1.0f);
    const int32 Tip = Cone.AppendVertex(FVector3d(0, 0, Height * 0.5));
    const int32 BottomCenter = Cone.AppendVertex(FVector3d(0, 0, -Height * 0.5));
    TArray<int32> Ring;
    Ring.Reserve(Sides);
    for (int32 Index = 0; Index < Sides; ++Index)
    {
        const double Angle = (2.0 * static_cast<double>(PI) * Index) / Sides;
        Ring.Add(Cone.AppendVertex(FVector3d(
            Radius * FMath::Cos(Angle),
            Radius * FMath::Sin(Angle),
            -Height * 0.5)));
    }
    for (int32 Index = 0; Index < Sides; ++Index)
    {
        const int32 Next = (Index + 1) % Sides;
        Cone.AppendTriangle(Tip, Ring[Index], Ring[Next]);
        Cone.AppendTriangle(BottomCenter, Ring[Next], Ring[Index]);
    }
    return Cone;
}
