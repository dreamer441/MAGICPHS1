#include "SpellGraphCompiler.h"

#include "SpellMotionTypes.h"
#include "SpellParameterRanges.h"
#include "SpellPatternTypes.h"
#include "SpellSpatialTypes.h"
#include "WorldCodexSubsystem.h"
#include "WorldCodexTypes.h"

namespace
{
    int32 MagnitudeLevel(const FName ConceptId)
    {
        if (ConceptId == FName(TEXT("value.magnitude.0"))) return 0;
        if (ConceptId == FName(TEXT("value.magnitude.1"))) return 1;
        if (ConceptId == FName(TEXT("value.magnitude.2"))) return 2;
        if (ConceptId == FName(TEXT("value.magnitude.3"))) return 3;
        if (ConceptId == FName(TEXT("value.magnitude.4"))) return 4;
        if (ConceptId == FName(TEXT("value.magnitude.5"))) return 5;
        return INDEX_NONE;
    }

    const FSpellGraphNode* FindNode(
        const TArray<FSpellGraphNode>& Nodes,
        const FGuid& NodeId)
    {
        for (const FSpellGraphNode& Node : Nodes)
        {
            if (Node.NodeId == NodeId)
            {
                return &Node;
            }
        }
        return nullptr;
    }

    int32 CountRoots(const TArray<FSpellGraphNode>& Nodes)
    {
        int32 Count = 0;
        for (const FSpellGraphNode& Node : Nodes)
        {
            if (!Node.ParentNodeId.IsValid())
            {
                ++Count;
            }
        }
        return Count;
    }
}

FSpellGraphCompileResult FSpellGraphCompiler::Compile(
    const TArray<FSpellGraphNode>& Nodes,
    const UWorldCodexSubsystem& Codex)
{
    FSpellGraphCompileResult Result;

    if (Nodes.Num() == 0)
    {
        Result.Message =
            FText::FromString(TEXT("Incomplete: Canvas is empty."));
        return Result;
    }

    if (CountRoots(Nodes) != 1)
    {
        Result.Message = FText::FromString(
            TEXT("Incomplete: Rune Canvas V1 requires exactly one Tier-I root."));
        return Result;
    }

    const FSpellGraphNode* Root = nullptr;

    for (const FSpellGraphNode& Node : Nodes)
    {
        if (!Node.ParentNodeId.IsValid())
        {
            Root = &Node;
            break;
        }
    }

    if (!Root ||
        Root->ConceptId != FName(TEXT("element.earth")))
    {
        Result.Message = FText::FromString(
            TEXT("Incomplete: V1 currently compiles Earth as the construction foundation."));
        return Result;
    }

    bool bHasShape = false;

    for (const FSpellGraphNode& Node : Nodes)
    {
        const FCodexEntry* Entry =
            Codex.FindEntry(Node.ConceptId);

        if (Entry &&
            Entry->Category == ECodexCategory::Shape)
        {
            bHasShape = true;
            break;
        }
    }

    if (!bHasShape)
    {
        Result.Message = FText::FromString(
            TEXT("Incomplete: add a Shape beneath the Earth construction."));
        return Result;
    }

    FSpellDefinition Definition;

    for (const FSpellGraphNode& Node : Nodes)
    {
        const FName Id = Node.ConceptId;

        if (Id == FName(TEXT("shape.sphere")))
        {
            Definition.Shape = ESpellShape::Sphere;
        }
        else if (Id == FName(TEXT("shape.cube")))
        {
            Definition.Shape = ESpellShape::Cube;
        }
        else if (Id == FName(TEXT("shape.cone")))
        {
            Definition.Shape = ESpellShape::Cone;
        }
        else if (Id == FName(TEXT("pattern.line")))
        {
            Definition.Pattern.Arrangement =
                ESpellArrangement::Line;
        }
        else if (Id == FName(TEXT("pattern.circle")))
        {
            Definition.Pattern.Arrangement =
                ESpellArrangement::Circle;
        }
    }

    for (const FSpellGraphNode& Node : Nodes)
    {
        if (!Node.ParentNodeId.IsValid())
        {
            continue;
        }

        const FCodexEntry* Entry =
            Codex.FindEntry(Node.ConceptId);

        if (!Entry ||
            Entry->Tier != ECodexTier::TierIII)
        {
            continue;
        }

        const FSpellGraphNode* ParentNode =
            FindNode(Nodes, Node.ParentNodeId);

        if (!ParentNode)
        {
            continue;
        }

        const FName Parent = ParentNode->ConceptId;
        const FName Value = Node.ConceptId;
        const int32 Level = MagnitudeLevel(Value);

        if (Level != INDEX_NONE)
        {
            const float N =
                static_cast<float>(Level) / 5.0f;

            if (Parent == FName(TEXT("motion.speed")))
            {
                Definition.SpeedMps = FMath::Lerp(
                    SpellParameterRanges::MinSpeedMps,
                    SpellParameterRanges::MaxSpeedMps,
                    N);
            }
            else if (Parent == FName(TEXT("spatial.distance")))
            {
                Definition.DistanceM = FMath::Lerp(
                    SpellParameterRanges::MinDistanceM,
                    SpellParameterRanges::MaxDistanceM,
                    N);
            }
            else if (Parent == FName(TEXT("material.density")))
            {
                Definition.Material.DensityKgPerM3 = FMath::Lerp(
                    SpellParameterRanges::MinDensityKgPerM3,
                    SpellParameterRanges::MaxDensityKgPerM3,
                    N);
            }
            else if (Parent == FName(TEXT("material.hardness")))
            {
                Definition.Material.Hardness = N;
            }
            else if (Parent == FName(TEXT("material.toughness")))
            {
                Definition.Material.Toughness = N;
            }
            else if (Parent == FName(TEXT("material.elasticity")))
            {
                Definition.Material.Restitution = N;
            }
            else if (Parent == FName(TEXT("shape.sphere.radius")))
            {
                Definition.ShapeDefinition.SphereRadiusCm = FMath::Lerp(
                    SpellParameterRanges::MinSphereRadiusCm,
                    SpellParameterRanges::MaxSphereRadiusCm,
                    N);
            }
            else if (Parent == FName(TEXT("shape.cube.x")))
            {
                Definition.ShapeDefinition.CubeXcm = FMath::Lerp(
                    SpellParameterRanges::MinCubeSideCm,
                    SpellParameterRanges::MaxCubeSideCm,
                    N);
            }
            else if (Parent == FName(TEXT("shape.cube.y")))
            {
                Definition.ShapeDefinition.CubeYcm = FMath::Lerp(
                    SpellParameterRanges::MinCubeSideCm,
                    SpellParameterRanges::MaxCubeSideCm,
                    N);
            }
            else if (Parent == FName(TEXT("shape.cube.z")))
            {
                Definition.ShapeDefinition.CubeZcm = FMath::Lerp(
                    SpellParameterRanges::MinCubeSideCm,
                    SpellParameterRanges::MaxCubeSideCm,
                    N);
            }
            else if (Parent == FName(TEXT("shape.cone.radius")))
            {
                Definition.ShapeDefinition.ConeRadiusCm = FMath::Lerp(
                    SpellParameterRanges::MinConeRadiusCm,
                    SpellParameterRanges::MaxConeRadiusCm,
                    N);
            }
            else if (Parent == FName(TEXT("shape.cone.height")))
            {
                Definition.ShapeDefinition.ConeHeightCm = FMath::Lerp(
                    SpellParameterRanges::MinConeHeightCm,
                    SpellParameterRanges::MaxConeHeightCm,
                    N);
            }
            else if (Parent == FName(TEXT("pattern.amount")))
            {
                Definition.Pattern.Amount = FMath::RoundToInt(
                    FMath::Lerp(
                        static_cast<float>(SpellPatternRanges::MinAmount),
                        static_cast<float>(SpellPatternRanges::MaxAmount),
                        N));
            }
            else if (Parent == FName(TEXT("pattern.spacing")))
            {
                Definition.Pattern.SpacingCm = FMath::Lerp(
                    SpellPatternRanges::MinSpacingCm,
                    SpellPatternRanges::MaxSpacingCm,
                    N);
            }
            else if (Parent == FName(TEXT("pattern.circle_radius")))
            {
                Definition.Pattern.CircleRadiusCm = FMath::Lerp(
                    SpellPatternRanges::MinCircleRadiusCm,
                    SpellPatternRanges::MaxCircleRadiusCm,
                    N);
            }

            continue;
        }

        if (Parent == FName(TEXT("motion.direction")))
        {
            if (Value == FName(TEXT("direction.backward")))
                Definition.MotionDirection = ESpellMotionDirection::Backward;
            else if (Value == FName(TEXT("direction.up")))
                Definition.MotionDirection = ESpellMotionDirection::Up;
            else if (Value == FName(TEXT("direction.down")))
                Definition.MotionDirection = ESpellMotionDirection::Down;
            else if (Value == FName(TEXT("direction.outward")))
                Definition.MotionDirection = ESpellMotionDirection::Outward;
            else if (Value == FName(TEXT("direction.inward")))
                Definition.MotionDirection = ESpellMotionDirection::Inward;
            else if (Value == FName(TEXT("direction.tangent")))
                Definition.MotionDirection = ESpellMotionDirection::Tangent;
            else
                Definition.MotionDirection = ESpellMotionDirection::Forward;

            continue;
        }

        if (Parent == FName(TEXT("spatial.orientation")))
        {
            if (Value == FName(TEXT("orientation.forward")))
                Definition.Orientation = ESpellOrientationAxis::Forward;
            else if (Value == FName(TEXT("orientation.right")))
                Definition.Orientation = ESpellOrientationAxis::Right;
            else
                Definition.Orientation = ESpellOrientationAxis::Up;

            continue;
        }

        if (Parent == FName(TEXT("pattern.line_axis")))
        {
            if (Value == FName(TEXT("axis.forward")))
                Definition.Pattern.LineAxis = ESpellPatternAxis::Forward;
            else if (Value == FName(TEXT("axis.up")))
                Definition.Pattern.LineAxis = ESpellPatternAxis::Up;
            else
                Definition.Pattern.LineAxis = ESpellPatternAxis::Right;

            continue;
        }

        if (Parent == FName(TEXT("pattern.orientation")))
        {
            if (Value == FName(TEXT("pattern.orientation.outward")))
                Definition.Pattern.InstanceOrientation = ESpellPatternOrientation::Outward;
            else if (Value == FName(TEXT("pattern.orientation.inward")))
                Definition.Pattern.InstanceOrientation = ESpellPatternOrientation::Inward;
            else if (Value == FName(TEXT("pattern.orientation.tangent")))
                Definition.Pattern.InstanceOrientation = ESpellPatternOrientation::Tangent;
            else
                Definition.Pattern.InstanceOrientation = ESpellPatternOrientation::Shared;

            continue;
        }
    }

    Result.Definition = Definition;
    Result.bSuccess = true;

    const TCHAR* ShapeName = TEXT("Sphere");
    if (Definition.Shape == ESpellShape::Cube) ShapeName = TEXT("Cube");
    else if (Definition.Shape == ESpellShape::Cone) ShapeName = TEXT("Cone");

    Result.Message = FText::FromString(FString::Printf(
        TEXT("VALID / APPLIED: EARTH -> %s   |   %d signs   |   changes feed the existing 3D preview and spell pipeline."),
        ShapeName,
        Nodes.Num()));

    return Result;
}
