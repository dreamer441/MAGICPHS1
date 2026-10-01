#include "Misc/AutomationTest.h"
#include "LiveSpellState.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLiveSpellStateGrammarTest, "AMADEUS.LiveSpell.Grammar", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLiveSpellStateGrammarTest::RunTest(const FString&)
{
    FLiveSpellState State;

    // A shape or modifier cannot begin construction by itself.
    State.SelectShape(ESpellShape::Sphere);
    State.SetParameterNormalized(ELiveSpellParameter::Speed, 0.5f);
    TestFalse(TEXT("Invalid order cannot cast"), State.CanCast());

    // The fallback shape remains stable while a construction has no explicit shape.
    State.SelectElement(ESpellElement::Earth);
    TestEqual(TEXT("Element selection advances generation"), State.GetGeneration(), 1u);
    TestEqual(TEXT("No explicit shape uses the persistent sphere default"), State.GetResolvedShape(), ESpellShape::Sphere);
    State.SelectShape(ESpellShape::Sphere);
    TestEqual(TEXT("Shape selection advances generation"), State.GetGeneration(), 2u);

    State.SetParameterNormalized(static_cast<ELiveSpellParameter>(255), 0.5f);
    TestFalse(TEXT("Unknown parameter does not satisfy the cast grammar"), State.CanCast());
    TestEqual(TEXT("Unknown parameter leaves shape-selection stage intact"), State.GetStage(), ELiveSpellStage::ShapeSelected);

    State.SetParameterNormalized(ELiveSpellParameter::Density, -1.0f);
    TestEqual(TEXT("Low modifier values clamp to zero"), State.GetParameterNormalized(ELiveSpellParameter::Density), 0.0f);
    State.SetParameterNormalized(ELiveSpellParameter::Density, 2.0f);
    TestEqual(TEXT("High modifier values clamp to one"), State.GetParameterNormalized(ELiveSpellParameter::Density), 1.0f);
    TestTrue(TEXT("Valid grammar can cast"), State.CanCast());
    TestEqual(TEXT("Speed remains zero without speed modifier"), State.Resolve(FSpellDefinition()).Definition.SpeedMps, 0.0f);

    // A new shape is a fresh construction and must clear old modifiers.
    State.SelectShape(ESpellShape::Sphere);
    TestEqual(TEXT("Reselecting a shape advances generation"), State.GetGeneration(), 3u);
    TestFalse(TEXT("Reselecting a shape clears modifiers"), State.HasModifier());
    TestFalse(TEXT("Reselecting a shape clears cast readiness"), State.CanCast());

    State.SetParameterNormalized(ELiveSpellParameter::Speed, 0.5f);
    TestTrue(TEXT("Explicit speed is positive"), State.Resolve(FSpellDefinition()).Definition.SpeedMps > 0.0f);

    // Selecting an element or resetting also starts a clean construction.
    State.SelectElement(ESpellElement::Earth);
    TestEqual(TEXT("Element reselection advances generation"), State.GetGeneration(), 4u);
    TestFalse(TEXT("Element reselection clears the explicit shape"), State.HasExplicitShape());
    TestFalse(TEXT("Element reselection clears modifiers"), State.HasModifier());
    State.Reset();
    TestEqual(TEXT("Reset advances generation"), State.GetGeneration(), 5u);
    TestFalse(TEXT("Reset clears construction"), State.HasConstruction());
    TestFalse(TEXT("Reset clears casting readiness"), State.CanCast());

    return true;
}
#endif
