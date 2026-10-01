#include "LiveSpellSessionSubsystem.h"
#include "SpellCreationSubsystem.h"
#include "SpellParameterRanges.h"
void ULiveSpellSessionSubsystem::Initialize(FSubsystemCollectionBase& Collection) { Super::Initialize(Collection); State.Reset(); }
void ULiveSpellSessionSubsystem::ResetSession() { PreparedSpellBase.Reset(); State.Reset(); }
void ULiveSpellSessionSubsystem::SelectEarth() { PreparedSpellBase.Reset(); State.SelectElement(ESpellElement::Earth); }
void ULiveSpellSessionSubsystem::SelectShape(EEarthSpellShape Shape) { State.SelectShape(Shape == EEarthSpellShape::Sphere ? ESpellShape::Sphere : Shape == EEarthSpellShape::Cube ? ESpellShape::Cube : ESpellShape::Cone); }
void ULiveSpellSessionSubsystem::LoadPreparedSpell(const FSpellDefinition& Spell)
{
    PreparedSpellBase = FSpellDefinitionAdapter::Resolve(Spell).Definition;
    State.Reset();
    State.SelectElement(PreparedSpellBase->Element);
    State.SelectShape(PreparedSpellBase->Shape);

    auto SetRange = [this](const ELiveSpellParameter Parameter, const float Value, const float MinValue, const float MaxValue)
    {
        State.SetParameterNormalized(Parameter, SpellParameterRanges::Normalize(Value, MinValue, MaxValue));
    };

    const FSpellDefinition& D = PreparedSpellBase.GetValue();
    SetRange(ELiveSpellParameter::SphereRadius, D.ShapeDefinition.SphereRadiusCm, SpellParameterRanges::MinSphereRadiusCm, SpellParameterRanges::MaxSphereRadiusCm);
    SetRange(ELiveSpellParameter::CubeX, D.ShapeDefinition.CubeXcm, SpellParameterRanges::MinCubeSideCm, SpellParameterRanges::MaxCubeSideCm);
    SetRange(ELiveSpellParameter::CubeY, D.ShapeDefinition.CubeYcm, SpellParameterRanges::MinCubeSideCm, SpellParameterRanges::MaxCubeSideCm);
    SetRange(ELiveSpellParameter::CubeZ, D.ShapeDefinition.CubeZcm, SpellParameterRanges::MinCubeSideCm, SpellParameterRanges::MaxCubeSideCm);
    SetRange(ELiveSpellParameter::ConeRadius, D.ShapeDefinition.ConeRadiusCm, SpellParameterRanges::MinConeRadiusCm, SpellParameterRanges::MaxConeRadiusCm);
    SetRange(ELiveSpellParameter::ConeHeight, D.ShapeDefinition.ConeHeightCm, SpellParameterRanges::MinConeHeightCm, SpellParameterRanges::MaxConeHeightCm);
    SetRange(ELiveSpellParameter::Speed, D.SpeedMps, SpellParameterRanges::MinSpeedMps, SpellParameterRanges::MaxSpeedMps);
    SetRange(ELiveSpellParameter::Density, D.Material.DensityKgPerM3, SpellParameterRanges::MinDensityKgPerM3, SpellParameterRanges::MaxDensityKgPerM3);
    SetRange(ELiveSpellParameter::Distance, D.DistanceM, SpellParameterRanges::MinDistanceM, SpellParameterRanges::MaxDistanceM);
    State.SetParameterNormalized(ELiveSpellParameter::Hardness, FMath::Clamp(D.Material.Hardness, 0.0f, 1.0f));
    State.SetParameterNormalized(ELiveSpellParameter::Toughness, FMath::Clamp(D.Material.Toughness, 0.0f, 1.0f));
    State.SetParameterNormalized(ELiveSpellParameter::Elasticity, FMath::Clamp(D.Material.Restitution, 0.0f, 1.0f));
}
EEarthSpellShape ULiveSpellSessionSubsystem::GetResolvedShape() const { const ESpellShape Shape = State.HasExplicitShape() ? State.GetResolvedShape() : GetPersistentGenericDefaults().Shape; return Shape == ESpellShape::Sphere ? EEarthSpellShape::Sphere : Shape == ESpellShape::Cube ? EEarthSpellShape::Cube : EEarthSpellShape::Cone; }
void ULiveSpellSessionSubsystem::SetParameterNormalized(ELiveSpellParameter Parameter, float Value) { State.SetParameterNormalized(Parameter, Value); }
FSpellDefinition ULiveSpellSessionSubsystem::GetPersistentGenericDefaults() const { if (const UWorld* World = GetWorld()) if (const USpellCreationSubsystem* Creation = World->GetSubsystem<USpellCreationSubsystem>()) return Creation->GetStoredGenericSpellDefinition(); return FSpellDefinition(); }
FResolvedSpell ULiveSpellSessionSubsystem::ResolveGenericSpell() const { return State.Resolve(PreparedSpellBase.IsSet() ? PreparedSpellBase.GetValue() : GetPersistentGenericDefaults()); }
FEarthSpellDefinition ULiveSpellSessionSubsystem::ResolveSpell() const { return FSpellDefinitionAdapter::ToLegacyEarth(ResolveGenericSpell().Definition); }
