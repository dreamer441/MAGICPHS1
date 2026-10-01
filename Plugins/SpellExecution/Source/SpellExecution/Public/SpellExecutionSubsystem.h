#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "SpellExecutionSubsystem.generated.h"

/** Observable result from one explicit spell-execution attempt. */
UENUM()
enum class ESpellExecutionOutcome : uint8
{
    NoWorld,
    NoController,
    ExecutionSuspended,
    NoLiveConstruction,
    PlacementFailed,
    UnsupportedElement,
    SpawnFailed,
    Executed
};

/** Reports whether a cast attempt was consumed, without exposing realization details. */
struct SPELLEXECUTION_API FSpellExecutionResult
{
    ESpellExecutionOutcome Outcome = ESpellExecutionOutcome::NoWorld;
    bool bConsumedConstruction = false;

    static FSpellExecutionResult ForOutcome(
        const ESpellExecutionOutcome InOutcome,
        const bool bInConsumedConstruction = false)
    {
        return {
            InOutcome,
            InOutcome == ESpellExecutionOutcome::Executed && bInConsumedConstruction
        };
    }
};

/**
 * Owns the production SPACE-key execution boundary. It reads resolved live
 * state and dispatches explicitly to the active element realization module.
 */
UCLASS()
class SPELLEXECUTION_API USpellExecutionSubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Tick(float DeltaSeconds) override;
    virtual TStatId GetStatId() const override;

    /** Reference-counted ownership gate for UI/modal systems such as InnerRealm. */
    void SetExecutionSuspended(bool bInSuspended);
    bool IsExecutionSuspended() const { return ExecutionSuspensionDepth > 0; }

    /** Attempts one cast without consuming live construction until realization succeeds. */
    FSpellExecutionResult ExecuteLiveSpell();

private:
    int32 ExecutionSuspensionDepth = 0;
};
