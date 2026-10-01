#include "Misc/AutomationTest.h"
#include "SpellExecutionSubsystem.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSpellExecutionResultGuardsTest,
    "AMADEUS.SpellExecution.ResultGuards",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSpellExecutionResultGuardsTest::RunTest(const FString&)
{
    USpellExecutionSubsystem* Execution = NewObject<USpellExecutionSubsystem>();
    TestNotNull(TEXT("Execution subsystem can be constructed for guard testing"), Execution);
    if (!Execution)
    {
        return false;
    }

    const FSpellExecutionResult NoWorldResult = Execution->ExecuteLiveSpell();
    TestEqual(TEXT("A worldless execution is rejected"), NoWorldResult.Outcome, ESpellExecutionOutcome::NoWorld);
    TestFalse(TEXT("A worldless execution never consumes construction"), NoWorldResult.bConsumedConstruction);

    Execution->SetExecutionSuspended(true);
    const FSpellExecutionResult SuspendedResult = Execution->ExecuteLiveSpell();
    TestEqual(TEXT("Suspension wins before all other execution guards"), SuspendedResult.Outcome, ESpellExecutionOutcome::ExecutionSuspended);
    TestFalse(TEXT("Suspension never consumes construction"), SuspendedResult.bConsumedConstruction);
    Execution->SetExecutionSuspended(false);

    const FSpellExecutionResult GuardResult =
        FSpellExecutionResult::ForOutcome(ESpellExecutionOutcome::NoLiveConstruction, true);

    TestEqual(TEXT("No live construction is reported"), GuardResult.Outcome, ESpellExecutionOutcome::NoLiveConstruction);
    TestFalse(TEXT("A guard cannot consume construction even if requested"), GuardResult.bConsumedConstruction);

    const FSpellExecutionResult ExecutedResult =
        FSpellExecutionResult::ForOutcome(ESpellExecutionOutcome::Executed, true);
    TestEqual(TEXT("Successful realization is reported"), ExecutedResult.Outcome, ESpellExecutionOutcome::Executed);
    TestTrue(TEXT("Only success consumes construction"), ExecutedResult.bConsumedConstruction);
    return true;
}
#endif
