#pragma once

#include "CoreMinimal.h"
#include "LiveSpellTypes.h"
#include "SpellDefinition.h"

class LIVESPELLCASTING_API FLiveSpellState
{
public:
    void Reset();
    void SelectElement(ESpellElement Element);
    void SelectShape(ESpellShape Shape);
    void SetParameterNormalized(ELiveSpellParameter Parameter, float Value);
    ELiveSpellStage GetStage() const { return Stage; }
    bool HasConstruction() const { return Stage != ELiveSpellStage::Empty; }
    bool HasElement() const { return Element.IsSet(); }
    bool HasExplicitShape() const { return Shape.IsSet(); }
    bool HasModifier() const { return ParameterOverrides01.Num() > 0; }
    bool CanSelectShape() const { return HasElement(); }
    bool CanApplyModifier() const { return HasExplicitShape(); }
    bool CanCast() const { return HasElement() && HasExplicitShape() && HasModifier(); }
    bool IsParameterActive(ELiveSpellParameter Parameter) const { return ParameterOverrides01.Contains(Parameter); }
    float GetParameterNormalized(ELiveSpellParameter Parameter) const;
    ESpellShape GetResolvedShape() const { return Shape.Get(ESpellShape::Sphere); }
    FResolvedSpell Resolve(const FSpellDefinition& Defaults) const;
    uint32 GetGeneration() const { return Generation; }
private:
    ELiveSpellStage Stage = ELiveSpellStage::Empty;
    TOptional<ESpellElement> Element;
    TOptional<ESpellShape> Shape;
    TMap<ELiveSpellParameter, float> ParameterOverrides01;
    uint32 Generation = 0;
};
