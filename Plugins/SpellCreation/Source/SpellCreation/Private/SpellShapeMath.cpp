#include "SpellShapeMath.h"
#include "EarthSpellDefinition.h"

namespace SpellShapeMathPrivate
{
static float Positive(const float Value, const float Minimum, const float Fallback)
{
    return FMath::IsFinite(Value) ? FMath::Max(Value, Minimum) : Fallback;
}

static float Clamped(const float Value, const float Minimum, const float Maximum, const float Fallback)
{
    return FMath::IsFinite(Value) ? FMath::Clamp(Value, Minimum, Maximum) : Fallback;
}

static FSpellDefinition Normalize(FSpellDefinition Spell)
{
    const FSpellDefinition Defaults;
    Spell.ShapeDefinition.SphereRadiusCm = Positive(Spell.ShapeDefinition.SphereRadiusCm, 1.0f, Defaults.ShapeDefinition.SphereRadiusCm);
    Spell.ShapeDefinition.CubeXcm = Positive(Spell.ShapeDefinition.CubeXcm, 1.0f, Defaults.ShapeDefinition.CubeXcm);
    Spell.ShapeDefinition.CubeYcm = Positive(Spell.ShapeDefinition.CubeYcm, 1.0f, Defaults.ShapeDefinition.CubeYcm);
    Spell.ShapeDefinition.CubeZcm = Positive(Spell.ShapeDefinition.CubeZcm, 1.0f, Defaults.ShapeDefinition.CubeZcm);
    Spell.ShapeDefinition.ConeRadiusCm = Positive(Spell.ShapeDefinition.ConeRadiusCm, 1.0f, Defaults.ShapeDefinition.ConeRadiusCm);
    Spell.ShapeDefinition.ConeHeightCm = Positive(Spell.ShapeDefinition.ConeHeightCm, 1.0f, Defaults.ShapeDefinition.ConeHeightCm);
    Spell.DistanceM = Positive(Spell.DistanceM, 0.0f, Defaults.DistanceM);
    Spell.Pattern.Amount = FMath::Clamp(
        Spell.Pattern.Amount,
        SpellPatternRanges::MinAmount,
        SpellPatternRanges::MaxAmount);
    Spell.Pattern.SpacingCm = Clamped(
        Spell.Pattern.SpacingCm,
        SpellPatternRanges::MinSpacingCm,
        SpellPatternRanges::MaxSpacingCm,
        Defaults.Pattern.SpacingCm);
    Spell.Pattern.CircleRadiusCm = Clamped(
        Spell.Pattern.CircleRadiusCm,
        SpellPatternRanges::MinCircleRadiusCm,
        SpellPatternRanges::MaxCircleRadiusCm,
        Defaults.Pattern.CircleRadiusCm);

    switch (Spell.Pattern.InstanceOrientation)
    {
        case ESpellPatternOrientation::Shared:
        case ESpellPatternOrientation::Outward:
        case ESpellPatternOrientation::Inward:
        case ESpellPatternOrientation::Tangent:
            break;
        default:
            Spell.Pattern.InstanceOrientation = ESpellPatternOrientation::Shared;
            break;
    }

    switch (Spell.MotionDirection)
    {
        case ESpellMotionDirection::Forward:
        case ESpellMotionDirection::Backward:
        case ESpellMotionDirection::Up:
        case ESpellMotionDirection::Down:
        case ESpellMotionDirection::Outward:
        case ESpellMotionDirection::Inward:
        case ESpellMotionDirection::Tangent:
            break;
        default:
            Spell.MotionDirection = ESpellMotionDirection::Forward;
            break;
    }

    Spell.SpeedMps = Positive(Spell.SpeedMps, 0.0f, Defaults.SpeedMps);
    Spell.Material.DensityKgPerM3 = Positive(Spell.Material.DensityKgPerM3, 1.0f, Defaults.Material.DensityKgPerM3);
    Spell.Material.Hardness = Clamped(Spell.Material.Hardness, 0.0f, 1.0f, Defaults.Material.Hardness);
    Spell.Material.Toughness = Clamped(Spell.Material.Toughness, 0.0f, 1.0f, Defaults.Material.Toughness);
    Spell.Material.Restitution = Clamped(Spell.Material.Restitution, 0.0f, 1.0f, Defaults.Material.Restitution);
    Spell.Material.Cohesion = Clamped(Spell.Material.Cohesion, 0.0f, 1.0f, Defaults.Material.Cohesion);
    Spell.Material.Rigidity = Clamped(Spell.Material.Rigidity, 0.0f, 1.0f, Defaults.Material.Rigidity);
    Spell.Material.Plasticity = Clamped(Spell.Material.Plasticity, 0.0f, 1.0f, Defaults.Material.Plasticity);
    Spell.Material.Friction = Clamped(Spell.Material.Friction, 0.0f, 2.0f, Defaults.Material.Friction);
    Spell.Material.TemperatureC = FMath::IsFinite(Spell.Material.TemperatureC) ? Spell.Material.TemperatureC : Defaults.Material.TemperatureC;
    return Spell;
}
}

using namespace SpellShapeMathPrivate;

float FSpellShapeMath::CalculateVolumeM3(const FSpellDefinition& Definition)
{
    const FSpellShapeDefinition& S = Definition.ShapeDefinition;
    switch (Definition.Shape)
    {
    case ESpellShape::Sphere: { const float R = Positive(S.SphereRadiusCm, .5f, 22.f) * .01f; return 4.f * PI * R * R * R / 3.f; }
    case ESpellShape::Cube: return Positive(S.CubeXcm, 1.f, 100.f) * Positive(S.CubeYcm, 1.f, 100.f) * Positive(S.CubeZcm, 1.f, 100.f) / 1000000.f;
    default: { const float R = Positive(S.ConeRadiusCm, .5f, 40.f) * .01f; const float H = Positive(S.ConeHeightCm, 1.f, 120.f) * .01f; return PI * R * R * H / 3.f; }
    }
}
float FSpellShapeMath::CalculateDensityKgPerM3(const FSpellDefinition& D) { return Positive(D.Material.DensityKgPerM3, 1.f, 1600.f); }
float FSpellShapeMath::CalculateMassKg(const FSpellDefinition& D) { return FMath::Max(CalculateDensityKgPerM3(D) * CalculateVolumeM3(D), .01f); }
float FSpellShapeMath::CalculateContactRadiusCm(const FSpellDefinition& D)
{
    const FSpellShapeDefinition& S = D.ShapeDefinition;
    return D.Shape == ESpellShape::Sphere ? Positive(S.SphereRadiusCm,.5f,22.f) : D.Shape == ESpellShape::Cube ? .5f * FMath::Max(Positive(S.CubeYcm,1.f,100.f),Positive(S.CubeZcm,1.f,100.f)) : Positive(S.ConeRadiusCm,.5f,40.f);
}
FVector FSpellShapeMath::CalculateHalfExtentsCm(const FSpellDefinition& D)
{
    const FSpellShapeDefinition& S = D.ShapeDefinition;
    if (D.Shape == ESpellShape::Sphere) { const double R=Positive(S.SphereRadiusCm,.5f,22.f); return FVector(R, R, R); }
    if (D.Shape == ESpellShape::Cube) return FVector(
        static_cast<double>(Positive(S.CubeXcm,1.f,100.f)*.5f),
        static_cast<double>(Positive(S.CubeYcm,1.f,100.f)*.5f),
        static_cast<double>(Positive(S.CubeZcm,1.f,100.f)*.5f));
    const double R=Positive(S.ConeRadiusCm,.5f,40.f); return FVector(R, R, static_cast<double>(Positive(S.ConeHeightCm,1.f,120.f)*.5f));
}
FSpellDefinition FSpellDefinitionAdapter::FromLegacyEarth(const FEarthSpellDefinition& L)
{
    FSpellDefinition D; D.Shape = L.Shape == EEarthSpellShape::Sphere ? ESpellShape::Sphere : L.Shape == EEarthSpellShape::Cube ? ESpellShape::Cube : ESpellShape::Cone;
    D.ShapeDefinition={L.SphereRadiusCm,L.CubeXcm,L.CubeYcm,L.CubeZcm,L.ConeRadiusCm,L.ConeHeightCm}; D.DistanceM=L.DistanceM; D.Orientation=L.Orientation; D.Pattern.Amount=L.Amount; D.Pattern.Arrangement=L.Arrangement; D.Pattern.LineAxis=L.PatternAxis; D.Pattern.InstanceOrientation=L.PatternOrientation; D.MotionDirection=L.MotionDirection; D.Pattern.SpacingCm=L.SpacingCm; D.Pattern.CircleRadiusCm=L.CircleRadiusCm; D.SpeedMps=L.SpeedMps;
    D.Material.DensityKgPerM3=L.DensityKgPerM3; D.Material.Hardness=L.Hardness; D.Material.Toughness=L.Toughness; D.Material.Restitution=L.Elasticity; D.Material.Cohesion=L.Cohesion; D.Material.Rigidity=L.Rigidity; return D;
}
FEarthSpellDefinition FSpellDefinitionAdapter::ToLegacyEarth(const FSpellDefinition& In)
{
    const FSpellDefinition D=SpellShapeMathPrivate::Normalize(In); FEarthSpellDefinition L; L.Shape=D.Shape==ESpellShape::Sphere?EEarthSpellShape::Sphere:D.Shape==ESpellShape::Cube?EEarthSpellShape::Cube:EEarthSpellShape::Cone;
    L.SphereRadiusCm=D.ShapeDefinition.SphereRadiusCm; L.CubeXcm=D.ShapeDefinition.CubeXcm; L.CubeYcm=D.ShapeDefinition.CubeYcm; L.CubeZcm=D.ShapeDefinition.CubeZcm; L.ConeRadiusCm=D.ShapeDefinition.ConeRadiusCm; L.ConeHeightCm=D.ShapeDefinition.ConeHeightCm; L.DistanceM=D.DistanceM; L.Orientation=D.Orientation; L.Amount=D.Pattern.Amount; L.Arrangement=D.Pattern.Arrangement; L.PatternAxis=D.Pattern.LineAxis; L.PatternOrientation=D.Pattern.InstanceOrientation; L.MotionDirection=D.MotionDirection; L.SpacingCm=D.Pattern.SpacingCm; L.CircleRadiusCm=D.Pattern.CircleRadiusCm; L.SpeedMps=D.SpeedMps; L.DensityKgPerM3=D.Material.DensityKgPerM3; L.Hardness=D.Material.Hardness; L.Toughness=D.Material.Toughness; L.Elasticity=D.Material.Restitution; L.Cohesion=D.Material.Cohesion; L.Rigidity=D.Material.Rigidity; L.MassKg=FSpellShapeMath::CalculateMassKg(D); return L;
}
FResolvedSpell FSpellDefinitionAdapter::Resolve(const FSpellDefinition& In)
{
    FResolvedSpell R; R.Definition=SpellShapeMathPrivate::Normalize(In); R.Body.Material=R.Definition.Material; R.Body.VolumeM3=FSpellShapeMath::CalculateVolumeM3(R.Definition); R.Body.MassKg=FSpellShapeMath::CalculateMassKg(R.Definition); R.Body.ContactRadiusCm=FSpellShapeMath::CalculateContactRadiusCm(R.Definition); R.Body.VelocityCmS=FVector::ZeroVector; R.Body.bAffectedByGravity=true; return R;
}
