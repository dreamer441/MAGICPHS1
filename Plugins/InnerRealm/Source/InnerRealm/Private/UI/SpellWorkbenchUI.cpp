#include "InnerRealmSubsystem.h"

#include "SpellCastingBindingSubsystem.h"
#include "SpellCreationSubsystem.h"
#include "SpellParameterRanges.h"
#include "SpellShapeMath.h"

namespace InnerRealmEditor
{
    static void GetDimensionRange(
        const ESpellShape Shape,
        const int32 Index,
        float& OutMin,
        float& OutMax)
    {
        switch (Shape)
        {
            case ESpellShape::Sphere:
                OutMin = SpellParameterRanges::MinSphereRadiusCm;
                OutMax = SpellParameterRanges::MaxSphereRadiusCm;
                break;

            case ESpellShape::Cube:
                OutMin = SpellParameterRanges::MinCubeSideCm;
                OutMax = SpellParameterRanges::MaxCubeSideCm;
                break;

            case ESpellShape::Cone:
            default:
                if (Index == 0)
                {
                    OutMin = SpellParameterRanges::MinConeRadiusCm;
                    OutMax = SpellParameterRanges::MaxConeRadiusCm;
                }
                else
                {
                    OutMin = SpellParameterRanges::MinConeHeightCm;
                    OutMax = SpellParameterRanges::MaxConeHeightCm;
                }
                break;
        }
    }
}

FSpellDefinition UInnerRealmSubsystem::ReadSpell() const
{
    if (const USpellCreationSubsystem* SpellCreation = GetSpellCreation())
    {
        return SpellCreation->GetStoredGenericSpellDefinition();
    }
    return FSpellDefinition();
}

void UInnerRealmSubsystem::WriteSpell(const FSpellDefinition& Spell)
{
    if (USpellCreationSubsystem* SpellCreation = GetSpellCreation())
    {
        SpellCreation->SetStoredGenericSpellDefinition(Spell);
    }
}

void UInnerRealmSubsystem::SetShape(const ESpellShape Shape)
{
    FSpellDefinition Spell = ReadSpell();
    Spell.Shape = Shape;
    WriteSpell(Spell);
}

FText UInnerRealmSubsystem::GetShapeName() const
{
    switch (ReadSpell().Shape)
    {
        case ESpellShape::Sphere: return FText::FromString(TEXT("SPHERE"));
        case ESpellShape::Cube:   return FText::FromString(TEXT("CUBE"));
        case ESpellShape::Cone:   return FText::FromString(TEXT("CONE"));
        default:                       return FText::FromString(TEXT("EARTH FORM"));
    }
}

FInputChord UInnerRealmSubsystem::GetBindingChord(const ESpellLiveAction Action) const
{
    if (const USpellCastingBindingSubsystem* Bindings = GetSpellBindings())
    {
        const FKey Key = Bindings->GetBinding(Action);
        if (Key.IsValid())
        {
            return FInputChord(Key);
        }
    }
    return FInputChord();
}

void UInnerRealmSubsystem::HandleBindingSelected(const ESpellLiveAction Action, const FInputChord& Chord)
{
    if (Chord.Key == EKeys::Tab)
    {
        ExitRealm();
        return;
    }

    USpellCastingBindingSubsystem* Bindings = GetSpellBindings();
    if (!Bindings)
    {
        return;
    }

    if (!Chord.Key.IsValid())
    {
        Bindings->ClearBinding(Action);
        return;
    }

    // Pressing the same assigned key again is the explicit empty/unassign gesture.
    if (Bindings->GetBinding(Action) == Chord.Key)
    {
        Bindings->ClearBinding(Action);
        return;
    }

    if (!Bindings->SetBinding(Action, Chord.Key) && GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 1.5f, FColor::Yellow,
            TEXT("That key is reserved for movement/camera/casting."));
    }
}

FText UInnerRealmSubsystem::GetCurrentSpellSummary() const
{
    const FSpellDefinition Spell = ReadSpell();
    const TCHAR* Orientation = TEXT("UP / Z");
    switch (Spell.Orientation)
    {
        case ESpellOrientationAxis::Forward: Orientation = TEXT("FORWARD / X"); break;
        case ESpellOrientationAxis::Right:   Orientation = TEXT("RIGHT / Y"); break;
        case ESpellOrientationAxis::Up:
        default:                             Orientation = TEXT("UP / Z"); break;
    }

    return FText::FromString(FString::Printf(
        TEXT("EARTH / %s / %s   |   x%d %s   |   speed %.0f m/s   |   density %.0f kg/m3   |   mass %.0f kg   |   distance %.1f m"),
        *GetShapeName().ToString(),
        Orientation,
        Spell.Pattern.Amount,
        Spell.Pattern.Amount > 1
            ? (Spell.Pattern.Arrangement == ESpellArrangement::Circle ? TEXT("CIRCLE") : TEXT("LINE"))
            : TEXT("SINGLE"),
        Spell.SpeedMps,
        Spell.Material.DensityKgPerM3,
        FSpellShapeMath::CalculateMassKg(Spell),
        Spell.DistanceM));
}

float UInnerRealmSubsystem::GetSpeedSlider() const
{
    return SpellParameterRanges::Normalize(ReadSpell().SpeedMps, SpellParameterRanges::MinSpeedMps, SpellParameterRanges::MaxSpeedMps);
}

float UInnerRealmSubsystem::GetDistanceSlider() const
{
    return SpellParameterRanges::Normalize(ReadSpell().DistanceM, SpellParameterRanges::MinDistanceM, SpellParameterRanges::MaxDistanceM);
}

float UInnerRealmSubsystem::GetDensitySlider() const
{
    return SpellParameterRanges::Normalize(ReadSpell().Material.DensityKgPerM3, SpellParameterRanges::MinDensityKgPerM3, SpellParameterRanges::MaxDensityKgPerM3);
}

float UInnerRealmSubsystem::GetHardnessSlider() const { return ReadSpell().Material.Hardness; }

float UInnerRealmSubsystem::GetToughnessSlider() const { return ReadSpell().Material.Toughness; }

float UInnerRealmSubsystem::GetElasticitySlider() const { return ReadSpell().Material.Restitution; }

void UInnerRealmSubsystem::SetSpeedSlider(const float Value)
{
    FSpellDefinition Spell = ReadSpell();
    Spell.SpeedMps = SpellParameterRanges::Denormalize(Value, SpellParameterRanges::MinSpeedMps, SpellParameterRanges::MaxSpeedMps);
    WriteSpell(Spell);
}

void UInnerRealmSubsystem::SetDistanceSlider(const float Value)
{
    FSpellDefinition Spell = ReadSpell();
    Spell.DistanceM = SpellParameterRanges::Denormalize(Value, SpellParameterRanges::MinDistanceM, SpellParameterRanges::MaxDistanceM);
    WriteSpell(Spell);
}

void UInnerRealmSubsystem::SetDensitySlider(const float Value)
{
    FSpellDefinition Spell = ReadSpell();
    Spell.Material.DensityKgPerM3 = SpellParameterRanges::Denormalize(Value, SpellParameterRanges::MinDensityKgPerM3, SpellParameterRanges::MaxDensityKgPerM3);
    WriteSpell(Spell);
}

void UInnerRealmSubsystem::SetHardnessSlider(const float Value)
{
    FSpellDefinition Spell = ReadSpell();
    Spell.Material.Hardness = FMath::Clamp(Value, 0.0f, 1.0f);
    WriteSpell(Spell);
}

void UInnerRealmSubsystem::SetToughnessSlider(const float Value)
{
    FSpellDefinition Spell = ReadSpell();
    Spell.Material.Toughness = FMath::Clamp(Value, 0.0f, 1.0f);
    WriteSpell(Spell);
}

void UInnerRealmSubsystem::SetElasticitySlider(const float Value)
{
    FSpellDefinition Spell = ReadSpell();
    Spell.Material.Restitution = FMath::Clamp(Value, 0.0f, 1.0f);
    WriteSpell(Spell);
}

float UInnerRealmSubsystem::GetDimensionSlider(const int32 Index) const
{
    const FSpellDefinition Spell = ReadSpell();
    float Value = Spell.ShapeDefinition.SphereRadiusCm;

    if (Spell.Shape == ESpellShape::Cube)
    {
        Value = Index == 0 ? Spell.ShapeDefinition.CubeXcm : (Index == 1 ? Spell.ShapeDefinition.CubeYcm : Spell.ShapeDefinition.CubeZcm);
    }
    else if (Spell.Shape == ESpellShape::Cone)
    {
        Value = Index == 0 ? Spell.ShapeDefinition.ConeRadiusCm : Spell.ShapeDefinition.ConeHeightCm;
    }

    float Min = 0.0f;
    float Max = 1.0f;
    InnerRealmEditor::GetDimensionRange(Spell.Shape, Index, Min, Max);
    return SpellParameterRanges::Normalize(Value, Min, Max);
}

void UInnerRealmSubsystem::SetDimensionSlider(const int32 Index, const float SliderValue)
{
    FSpellDefinition Spell = ReadSpell();
    float Min = 0.0f;
    float Max = 1.0f;
    InnerRealmEditor::GetDimensionRange(Spell.Shape, Index, Min, Max);
    const float Value = SpellParameterRanges::Denormalize(SliderValue, Min, Max);

    if (Spell.Shape == ESpellShape::Sphere)
    {
        Spell.ShapeDefinition.SphereRadiusCm = Value;
    }
    else if (Spell.Shape == ESpellShape::Cube)
    {
        if (Index == 0) Spell.ShapeDefinition.CubeXcm = Value;
        else if (Index == 1) Spell.ShapeDefinition.CubeYcm = Value;
        else Spell.ShapeDefinition.CubeZcm = Value;
    }
    else
    {
        if (Index == 0) Spell.ShapeDefinition.ConeRadiusCm = Value;
        else Spell.ShapeDefinition.ConeHeightCm = Value;
    }

    WriteSpell(Spell);
}

EVisibility UInnerRealmSubsystem::GetDimensionVisibility(const int32 Index) const
{
    const ESpellShape Shape = ReadSpell().Shape;
    if (Shape == ESpellShape::Sphere && Index > 0) return EVisibility::Collapsed;
    if (Shape == ESpellShape::Cone && Index > 1) return EVisibility::Collapsed;
    return EVisibility::Visible;
}

FText UInnerRealmSubsystem::GetDimensionLabel(const int32 Index) const
{
    const FSpellDefinition Spell = ReadSpell();
    FString Name;
    float Value = 0.0f;

    if (Spell.Shape == ESpellShape::Sphere)
    {
        Name = TEXT("Radius");
        Value = Spell.ShapeDefinition.SphereRadiusCm;
    }
    else if (Spell.Shape == ESpellShape::Cube)
    {
        if (Index == 0) { Name = TEXT("X"); Value = Spell.ShapeDefinition.CubeXcm; }
        else if (Index == 1) { Name = TEXT("Y"); Value = Spell.ShapeDefinition.CubeYcm; }
        else { Name = TEXT("Z"); Value = Spell.ShapeDefinition.CubeZcm; }
    }
    else
    {
        if (Index == 0) { Name = TEXT("Radius"); Value = Spell.ShapeDefinition.ConeRadiusCm; }
        else { Name = TEXT("Height"); Value = Spell.ShapeDefinition.ConeHeightCm; }
    }

    return FText::FromString(FString::Printf(TEXT("%s: %.0f cm"), *Name, Value));
}
