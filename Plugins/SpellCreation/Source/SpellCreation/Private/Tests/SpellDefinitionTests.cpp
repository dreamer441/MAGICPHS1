#include "Misc/AutomationTest.h"
#include "EarthSpellDefinition.h"
#include "SpellDefinition.h"
#include "SpellShapeMath.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSpellDefinitionRoundTripTest,
    "AMADEUS.SpellCreation.LegacyRoundTrip",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSpellDefinitionRoundTripTest::RunTest(const FString&)
{
    FEarthSpellDefinition Legacy;
    Legacy.Shape = EEarthSpellShape::Cone;
    Legacy.SphereRadiusCm = 29.0f;
    Legacy.CubeXcm = 80.0f;
    Legacy.CubeYcm = 90.0f;
    Legacy.CubeZcm = 100.0f;
    Legacy.ConeRadiusCm = 63.0f;
    Legacy.ConeHeightCm = 180.0f;
    Legacy.DistanceM = 7.5f;
    Legacy.SpeedMps = 0.0f;
    Legacy.DensityKgPerM3 = 1875.0f;
    Legacy.MassKg = 999.0f;
    Legacy.Hardness = 0.31f;
    Legacy.Toughness = 0.40f;
    Legacy.Elasticity = 0.12f;
    Legacy.Cohesion = 0.47f;
    Legacy.Rigidity = 0.66f;

    const FSpellDefinition Generic = FSpellDefinitionAdapter::FromLegacyEarth(Legacy);
    const FEarthSpellDefinition Restored = FSpellDefinitionAdapter::ToLegacyEarth(Generic);
    const FResolvedSpell Resolved = FSpellDefinitionAdapter::Resolve(Generic);
    TestEqual(TEXT("Element"), Generic.Element, ESpellElement::Earth);
    TestEqual(TEXT("Generic shape"), Generic.Shape, ESpellShape::Cone);
    TestEqual(TEXT("Generic sphere"), Generic.ShapeDefinition.SphereRadiusCm, Legacy.SphereRadiusCm);
    TestEqual(TEXT("Generic cube X"), Generic.ShapeDefinition.CubeXcm, Legacy.CubeXcm);
    TestEqual(TEXT("Generic cube Y"), Generic.ShapeDefinition.CubeYcm, Legacy.CubeYcm);
    TestEqual(TEXT("Generic cube Z"), Generic.ShapeDefinition.CubeZcm, Legacy.CubeZcm);
    TestEqual(TEXT("Generic cone radius"), Generic.ShapeDefinition.ConeRadiusCm, Legacy.ConeRadiusCm);
    TestEqual(TEXT("Generic cone height"), Generic.ShapeDefinition.ConeHeightCm, Legacy.ConeHeightCm);
    TestEqual(TEXT("Generic distance"), Generic.DistanceM, Legacy.DistanceM);
    TestEqual(TEXT("Generic speed"), Generic.SpeedMps, Legacy.SpeedMps);
    TestEqual(TEXT("Generic density"), Generic.Material.DensityKgPerM3, Legacy.DensityKgPerM3);
    TestEqual(TEXT("Generic hardness"), Generic.Material.Hardness, Legacy.Hardness);
    TestEqual(TEXT("Generic toughness"), Generic.Material.Toughness, Legacy.Toughness);
    TestEqual(TEXT("Generic restitution"), Generic.Material.Restitution, Legacy.Elasticity);
    TestEqual(TEXT("Generic cohesion"), Generic.Material.Cohesion, Legacy.Cohesion);
    TestEqual(TEXT("Generic rigidity"), Generic.Material.Rigidity, Legacy.Rigidity);
    TestEqual(TEXT("Shape"), Restored.Shape, Legacy.Shape);
    TestEqual(TEXT("Sphere"), Restored.SphereRadiusCm, Legacy.SphereRadiusCm);
    TestEqual(TEXT("Cube X"), Restored.CubeXcm, Legacy.CubeXcm);
    TestEqual(TEXT("Cube Y"), Restored.CubeYcm, Legacy.CubeYcm);
    TestEqual(TEXT("Cube Z"), Restored.CubeZcm, Legacy.CubeZcm);
    TestEqual(TEXT("Cone radius"), Restored.ConeRadiusCm, Legacy.ConeRadiusCm);
    TestEqual(TEXT("Cone height"), Restored.ConeHeightCm, Legacy.ConeHeightCm);
    TestEqual(TEXT("Distance"), Restored.DistanceM, Legacy.DistanceM);
    TestEqual(TEXT("Speed"), Restored.SpeedMps, Legacy.SpeedMps);
    TestEqual(TEXT("Density"), Restored.DensityKgPerM3, Legacy.DensityKgPerM3);
    TestEqual(TEXT("Hardness"), Restored.Hardness, Legacy.Hardness);
    TestEqual(TEXT("Toughness"), Restored.Toughness, Legacy.Toughness);
    TestEqual(TEXT("Elasticity"), Restored.Elasticity, Legacy.Elasticity);
    TestEqual(TEXT("Cohesion"), Restored.Cohesion, Legacy.Cohesion);
    TestEqual(TEXT("Rigidity"), Restored.Rigidity, Legacy.Rigidity);
    TestTrue(TEXT("Legacy mass is discarded"), !FMath::IsNearlyEqual(Resolved.Body.MassKg, Legacy.MassKg));
    TestTrue(TEXT("Legacy cache is recalculated"), FMath::IsNearlyEqual(Restored.MassKg, Resolved.Body.MassKg));
    TestEqual(TEXT("Body material"), Resolved.Body.Material.Toughness, Generic.Material.Toughness);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSpellDefinitionPhysicalDerivationTest,
    "AMADEUS.SpellCreation.PhysicalDerivation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSpellDefinitionPhysicalDerivationTest::RunTest(const FString&)
{
    FSpellDefinition Definition;
    Definition.Shape = ESpellShape::Sphere;
    Definition.Material.DensityKgPerM3 = 1600.0f;
    const FResolvedSpell Resolved = FSpellDefinitionAdapter::Resolve(Definition);
    const float Volume = FSpellShapeMath::CalculateVolumeM3(Definition);
    TestTrue(TEXT("Volume derives from shape"), FMath::IsNearlyEqual(Resolved.Body.VolumeM3, Volume));
    TestTrue(TEXT("Mass derives from density and volume"), FMath::IsNearlyEqual(Resolved.Body.MassKg, 1600.0f * Volume));
    TestTrue(TEXT("Contact radius derives from shape"), FMath::IsNearlyEqual(Resolved.Body.ContactRadiusCm, FSpellShapeMath::CalculateContactRadiusCm(Definition)));
    TestTrue(TEXT("Half extents available on demand"), !FSpellShapeMath::CalculateHalfExtentsCm(Definition).IsZero());
    TestTrue(TEXT("Velocity stays zero"), Resolved.Body.VelocityCmS.IsZero());
    TestTrue(TEXT("Gravity stays enabled"), Resolved.Body.bAffectedByGravity);
    TestEqual(TEXT("Generic default toughness"), Definition.Material.Toughness, 0.40f);
    return true;
}

#endif
