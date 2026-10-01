#include "LiveSpellState.h"
#include "SpellParameterRanges.h"

namespace
{
    /** Keeps corrupted or future enum values from becoming cast-valid modifiers. */
    bool IsSupportedParameter(const ELiveSpellParameter Parameter)
    {
        switch (Parameter)
        {
        case ELiveSpellParameter::SphereRadius:
        case ELiveSpellParameter::CubeX:
        case ELiveSpellParameter::CubeY:
        case ELiveSpellParameter::CubeZ:
        case ELiveSpellParameter::ConeRadius:
        case ELiveSpellParameter::ConeHeight:
        case ELiveSpellParameter::Speed:
        case ELiveSpellParameter::Density:
        case ELiveSpellParameter::Distance:
        case ELiveSpellParameter::Hardness:
        case ELiveSpellParameter::Toughness:
        case ELiveSpellParameter::Elasticity:
            return true;
        default:
            return false;
        }
    }
}

void FLiveSpellState::Reset() { Stage = ELiveSpellStage::Empty; Element.Reset(); Shape.Reset(); ParameterOverrides01.Reset(); ++Generation; }
void FLiveSpellState::SelectElement(ESpellElement InElement) { Element = InElement; Shape.Reset(); ParameterOverrides01.Reset(); Stage = ELiveSpellStage::ElementSelected; ++Generation; }
void FLiveSpellState::SelectShape(ESpellShape InShape) { if (!HasElement()) return; Shape = InShape; ParameterOverrides01.Reset(); Stage = ELiveSpellStage::ShapeSelected; ++Generation; }
void FLiveSpellState::SetParameterNormalized(ELiveSpellParameter Parameter, float Value) { if (!HasExplicitShape() || !IsSupportedParameter(Parameter)) return; ParameterOverrides01.Add(Parameter, FMath::Clamp(Value, 0.0f, 1.0f)); Stage = ELiveSpellStage::ModifierActive; }
float FLiveSpellState::GetParameterNormalized(ELiveSpellParameter Parameter) const { return ParameterOverrides01.FindRef(Parameter); }

FResolvedSpell FLiveSpellState::Resolve(const FSpellDefinition& Defaults) const
{
    FSpellDefinition Spell = Defaults;
    if (Element.IsSet()) Spell.Element = Element.GetValue();
    if (Shape.IsSet()) Spell.Shape = Shape.GetValue();
    Spell.SpeedMps = 0.0f;
    auto Apply = [this, &Spell](ELiveSpellParameter P, float& Field, float Min, float Max)
    {
        if (const float* Value = ParameterOverrides01.Find(P)) Field = SpellParameterRanges::Denormalize(*Value, Min, Max);
    };
    Apply(ELiveSpellParameter::SphereRadius, Spell.ShapeDefinition.SphereRadiusCm, SpellParameterRanges::MinSphereRadiusCm, SpellParameterRanges::MaxSphereRadiusCm);
    Apply(ELiveSpellParameter::CubeX, Spell.ShapeDefinition.CubeXcm, SpellParameterRanges::MinCubeSideCm, SpellParameterRanges::MaxCubeSideCm);
    Apply(ELiveSpellParameter::CubeY, Spell.ShapeDefinition.CubeYcm, SpellParameterRanges::MinCubeSideCm, SpellParameterRanges::MaxCubeSideCm);
    Apply(ELiveSpellParameter::CubeZ, Spell.ShapeDefinition.CubeZcm, SpellParameterRanges::MinCubeSideCm, SpellParameterRanges::MaxCubeSideCm);
    Apply(ELiveSpellParameter::ConeRadius, Spell.ShapeDefinition.ConeRadiusCm, SpellParameterRanges::MinConeRadiusCm, SpellParameterRanges::MaxConeRadiusCm);
    Apply(ELiveSpellParameter::ConeHeight, Spell.ShapeDefinition.ConeHeightCm, SpellParameterRanges::MinConeHeightCm, SpellParameterRanges::MaxConeHeightCm);
    Apply(ELiveSpellParameter::Speed, Spell.SpeedMps, SpellParameterRanges::MinSpeedMps, SpellParameterRanges::MaxSpeedMps);
    Apply(ELiveSpellParameter::Density, Spell.Material.DensityKgPerM3, SpellParameterRanges::MinDensityKgPerM3, SpellParameterRanges::MaxDensityKgPerM3);
    Apply(ELiveSpellParameter::Distance, Spell.DistanceM, SpellParameterRanges::MinDistanceM, SpellParameterRanges::MaxDistanceM);
    Apply(ELiveSpellParameter::Hardness, Spell.Material.Hardness, 0.0f, 1.0f);
    Apply(ELiveSpellParameter::Toughness, Spell.Material.Toughness, 0.0f, 1.0f);
    Apply(ELiveSpellParameter::Elasticity, Spell.Material.Restitution, 0.0f, 1.0f);
    return FSpellDefinitionAdapter::Resolve(Spell);
}
