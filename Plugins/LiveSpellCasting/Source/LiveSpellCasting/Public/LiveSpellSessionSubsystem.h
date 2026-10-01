#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "EarthSpellDefinition.h"
#include "LiveSpellState.h"
#include "LiveSpellSessionSubsystem.generated.h"
UCLASS()
class LIVESPELLCASTING_API ULiveSpellSessionSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    void ResetSession();
    ELiveSpellStage GetStage() const { return State.GetStage(); }
    bool HasConstruction() const { return State.HasConstruction(); }
    bool HasElement() const { return State.HasElement(); }
    bool HasExplicitShape() const { return State.HasExplicitShape(); }
    bool HasModifier() const { return State.HasModifier(); }
    bool CanSelectShape() const { return State.CanSelectShape(); }
    bool CanApplyModifier() const { return State.CanApplyModifier(); }
    bool CanCast() const { return State.CanCast(); }
    void SelectEarth();
    bool IsEarthExplicitlySelected() const { return State.HasElement(); }
    void SelectShape(EEarthSpellShape Shape);
    /** Loads a complete prepared definition into the transient live session. */
    void LoadPreparedSpell(const FSpellDefinition& Spell);
    bool HasShapeOverride() const { return State.HasExplicitShape(); }
    EEarthSpellShape GetResolvedShape() const;
    void SetParameterNormalized(ELiveSpellParameter Parameter, float NormalizedValue);
    bool IsParameterActive(ELiveSpellParameter Parameter) const { return State.IsParameterActive(Parameter); }
    float GetParameterNormalized(ELiveSpellParameter Parameter) const { return State.GetParameterNormalized(Parameter); }
    FEarthSpellDefinition ResolveSpell() const;
    void SelectElement(ESpellElement Element) { State.SelectElement(Element); }
    void SelectGenericShape(ESpellShape Shape) { State.SelectShape(Shape); }
    FResolvedSpell ResolveGenericSpell() const;
    uint32 GetGeneration() const { return State.GetGeneration(); }
private:
    FLiveSpellState State;
    /** Optional prepared spell base; manual Q/root selection clears it. */
    TOptional<FSpellDefinition> PreparedSpellBase;
    FSpellDefinition GetPersistentGenericDefaults() const;
};
