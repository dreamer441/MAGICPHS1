#pragma once

#include "CoreMinimal.h"
#include "InputCoreTypes.h"
#include "Subsystems/WorldSubsystem.h"
#include "SpellCastingBindingSubsystem.generated.h"

/**
 * Semantic live-casting actions.
 * Selector actions are taps; value actions are 0..3 second holds.
 * Shape dimensions are distinct actions so one physical key may intentionally
 * drive several compatible dimensions at once.
 */
UENUM(BlueprintType)
enum class ESpellLiveAction : uint8
{
    SelectEarth,
    SelectSphere,
    SelectCube,
    SelectCone,

    SphereRadius,
    CubeX,
    CubeY,
    CubeZ,
    ConeRadius,
    ConeHeight,

    Speed,
    Density,
    Distance,
    Hardness,
    Toughness,
    Elasticity
};

/**
 * Top-down live spell input node.
 *
 * - bindings are configuration only;
 * - holding a value key creates a temporary live override;
 * - unpressed values continue to use the TAB editor defaults;
 * - the same key may intentionally be used by multiple compatible value actions;
 * - pressing the already-assigned key in the binding picker clears that binding.
 */
UCLASS()
class SPELLCASTINGBINDINGS_API USpellCastingBindingSubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual TStatId GetStatId() const override;

    void SetTopDownLiveInputEnabled(bool bEnabled);
    void SetSuspended(bool bInSuspended);

    bool IsTopDownLiveInputEnabled() const { return bTopDownLiveInputEnabled; }
    bool IsSuspended() const { return bSuspended; }

    FKey GetBinding(ESpellLiveAction Action) const;
    FText GetBindingLabel(ESpellLiveAction Action) const;

    bool SetBinding(ESpellLiveAction Action, const FKey& Key);
    void ClearBinding(ESpellLiveAction Action);
    bool CanAssignKey(const FKey& Key) const;

    bool IsContinuousAction(ESpellLiveAction Action) const;
    float GetLiveValue01(ESpellLiveAction Action) const;

private:
    TMap<ESpellLiveAction, FKey> Bindings;
    TMap<ESpellLiveAction, float> ActiveHoldSeconds;
    TSet<FKey> KeysWaitingForRelease;

    bool bTopDownLiveInputEnabled = false;
    bool bSuspended = false;
    uint32 LastObservedSessionGeneration = 0;

    void ResetActiveHolds();
    void ApplyToggleAction(ESpellLiveAction Action);
    void ApplyContinuousValue(ESpellLiveAction Action, float NormalizedValue);
    bool IsActionApplicableToShape(ESpellLiveAction Action) const;
    bool IsShapeSelector(ESpellLiveAction Action) const;
};
