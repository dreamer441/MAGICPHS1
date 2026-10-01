#include "WorldCodexSubsystem.h"
#include <initializer_list>

namespace
{
    TArray<FName> Names(std::initializer_list<const TCHAR*> Values)
    {
        TArray<FName> Result;
        Result.Reserve(static_cast<int32>(Values.size()));
        for (const TCHAR* Value : Values)
        {
            Result.Add(FName(Value));
        }
        return Result;
    }

    FCodexEntry MakeEntry(
        const TCHAR* ConceptId,
        const TCHAR* DisplayName,
        const ECodexCategory Category,
        const TCHAR* Glyph,
        const TCHAR* Description,
        const TCHAR* OwningSystem,
        const ECodexImplementationState State,
        std::initializer_list<const TCHAR*> Provides = {},
        std::initializer_list<const TCHAR*> Requires = {},
        const TCHAR* DeveloperNotes = TEXT(""),
        const ECodexTier Tier = ECodexTier::TierII)
    {
        FCodexEntry Entry;
        Entry.ConceptId = FName(ConceptId);
        Entry.DisplayName = FText::FromString(DisplayName);
        Entry.Category = Category;
        Entry.Tier = Tier;
        Entry.Sign.Glyph = Glyph;
        Entry.Description = FText::FromString(Description);
        Entry.OwningSystem = FName(OwningSystem);
        Entry.ImplementationState = State;
        Entry.ProvidesCapabilities = Names(Provides);
        Entry.RequiresCapabilities = Names(Requires);
        Entry.DeveloperNotes = FText::FromString(DeveloperNotes);
        return Entry;
    }

    void Relate(
        FCodexEntry& Entry,
        const TCHAR* Relation,
        const TCHAR* TargetConceptId)
    {
        Entry.Relationships.Add(
            FCodexRelation(FName(Relation), FName(TargetConceptId)));
    }
}

void UWorldCodexSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    Entries.Reset();
    EntryIndexById.Reset();
    RegisterBuiltInVocabulary();
}

void UWorldCodexSubsystem::RegisterEntry(const FCodexEntry& Entry)
{
    if (Entry.ConceptId.IsNone())
    {
        return;
    }

    if (const int32* ExistingIndex = EntryIndexById.Find(Entry.ConceptId))
    {
        Entries[*ExistingIndex] = Entry;
        return;
    }

    const int32 NewIndex = Entries.Add(Entry);
    EntryIndexById.Add(Entry.ConceptId, NewIndex);
}

const FCodexEntry* UWorldCodexSubsystem::FindEntry(const FName ConceptId) const
{
    if (const int32* Index = EntryIndexById.Find(ConceptId))
    {
        return Entries.IsValidIndex(*Index) ? &Entries[*Index] : nullptr;
    }
    return nullptr;
}

TArray<const FCodexEntry*> UWorldCodexSubsystem::GetEntriesByCategory(
    const ECodexCategory Category) const
{
    TArray<const FCodexEntry*> Result;

    for (const FCodexEntry& Entry : Entries)
    {
        if (Entry.Category == Category)
        {
            Result.Add(&Entry);
        }
    }

    return Result;
}

bool UWorldCodexSubsystem::RequirementsSatisfied(
    const FCodexEntry& Entry,
    const TSet<FName>& AvailableCapabilities,
    TArray<FName>* OutMissingCapabilities) const
{
    if (OutMissingCapabilities)
    {
        OutMissingCapabilities->Reset();
    }

    bool bSatisfied = true;

    for (const FName Required : Entry.RequiresCapabilities)
    {
        if (!AvailableCapabilities.Contains(Required))
        {
            bSatisfied = false;
            if (OutMissingCapabilities)
            {
                OutMissingCapabilities->Add(Required);
            }
        }
    }

    return bSatisfied;
}

void UWorldCodexSubsystem::AddProvidedCapabilities(
    const FCodexEntry& Entry,
    TSet<FName>& InOutCapabilities) const
{
    for (const FName Capability : Entry.ProvidesCapabilities)
    {
        InOutCapabilities.Add(Capability);
    }
}

bool UWorldCodexSubsystem::TierAllowsConnection(
    const FCodexEntry& Parent,
    const FCodexEntry& Child) const
{
    return Parent.CanHaveChildren() && Child.CanHaveParent();
}

void UWorldCodexSubsystem::RegisterBuiltInVocabulary()
{
    // ---------------------------------------------------------------------
    // TIER I — FOUNDATIONS
    // ---------------------------------------------------------------------
    {
        FCodexEntry Entry = MakeEntry(
            TEXT("element.earth"),
            TEXT("Earth"),
            ECodexCategory::Element,
            TEXT("E"),
            TEXT("Solid-matter elemental foundation. Earth can form bounded rigid bodies with mass, collision and solid material properties."),
            TEXT("SpellCreation"),
            ECodexImplementationState::Implemented,
            {
                TEXT("construct.instanceable"),
                TEXT("construct.orientable"),
                TEXT("construct.placeable"),
                TEXT("construct.movable"),
                TEXT("geometry.bounded_solid"),
                TEXT("material.solid_properties"),
                TEXT("physics.rigid_body")
            },
            {},
            TEXT("Tier I construction root. Future Water, Air and Fire are expected to expose different capability sets rather than inheriting Earth geometry."),
            ECodexTier::TierI);

        Relate(Entry, TEXT("supports"), TEXT("shape.sphere"));
        Relate(Entry, TEXT("supports"), TEXT("shape.cube"));
        Relate(Entry, TEXT("supports"), TEXT("shape.cone"));
        RegisterEntry(Entry);
    }

    // ---------------------------------------------------------------------
    // TIER II — SHAPES
    // ---------------------------------------------------------------------
    {
        FCodexEntry Entry = MakeEntry(
            TEXT("shape.sphere"), TEXT("Sphere"),
            ECodexCategory::Shape, TEXT("O"),
            TEXT("Bounded spherical solid geometry."),
            TEXT("SpellCreation"), ECodexImplementationState::Implemented,
            { TEXT("shape.sphere"), TEXT("shape.has_radius") },
            { TEXT("geometry.bounded_solid") });

        Relate(Entry, TEXT("parameter"), TEXT("shape.sphere.radius"));
        RegisterEntry(Entry);
    }

    {
        FCodexEntry Entry = MakeEntry(
            TEXT("shape.cube"), TEXT("Cube"),
            ECodexCategory::Shape, TEXT("[]"),
            TEXT("Bounded rectangular solid geometry with independent X, Y and Z dimensions."),
            TEXT("SpellCreation"), ECodexImplementationState::Implemented,
            { TEXT("shape.cube"), TEXT("shape.has_xyz") },
            { TEXT("geometry.bounded_solid") });

        Relate(Entry, TEXT("parameter"), TEXT("shape.cube.x"));
        Relate(Entry, TEXT("parameter"), TEXT("shape.cube.y"));
        Relate(Entry, TEXT("parameter"), TEXT("shape.cube.z"));
        RegisterEntry(Entry);
    }

    {
        FCodexEntry Entry = MakeEntry(
            TEXT("shape.cone"), TEXT("Cone"),
            ECodexCategory::Shape, TEXT("^"),
            TEXT("Bounded conical solid geometry."),
            TEXT("SpellCreation"), ECodexImplementationState::Implemented,
            { TEXT("shape.cone"), TEXT("shape.has_radius"), TEXT("shape.has_height") },
            { TEXT("geometry.bounded_solid") });

        Relate(Entry, TEXT("parameter"), TEXT("shape.cone.radius"));
        Relate(Entry, TEXT("parameter"), TEXT("shape.cone.height"));
        RegisterEntry(Entry);
    }

    // ---------------------------------------------------------------------
    // TIER II — SHAPE PARAMETERS
    // ---------------------------------------------------------------------
    RegisterEntry(MakeEntry(
        TEXT("shape.sphere.radius"), TEXT("Sphere Radius"),
        ECodexCategory::ShapeParameter, TEXT("rO"),
        TEXT("Controls the radius of a sphere. A later Rune Canvas can attach a Tier III magnitude/value beneath it."),
        TEXT("SpellCreation"), ECodexImplementationState::Implemented,
        {}, { TEXT("shape.sphere") }));

    RegisterEntry(MakeEntry(
        TEXT("shape.cube.x"), TEXT("Cube X"),
        ECodexCategory::ShapeParameter, TEXT("X"),
        TEXT("Controls the cube local X dimension."),
        TEXT("SpellCreation"), ECodexImplementationState::Implemented,
        {}, { TEXT("shape.cube") }));

    RegisterEntry(MakeEntry(
        TEXT("shape.cube.y"), TEXT("Cube Y"),
        ECodexCategory::ShapeParameter, TEXT("Y"),
        TEXT("Controls the cube local Y dimension."),
        TEXT("SpellCreation"), ECodexImplementationState::Implemented,
        {}, { TEXT("shape.cube") }));

    RegisterEntry(MakeEntry(
        TEXT("shape.cube.z"), TEXT("Cube Z"),
        ECodexCategory::ShapeParameter, TEXT("Z"),
        TEXT("Controls the cube local Z dimension."),
        TEXT("SpellCreation"), ECodexImplementationState::Implemented,
        {}, { TEXT("shape.cube") }));

    RegisterEntry(MakeEntry(
        TEXT("shape.cone.radius"), TEXT("Cone Radius"),
        ECodexCategory::ShapeParameter, TEXT("r^"),
        TEXT("Controls the radius of the cone base."),
        TEXT("SpellCreation"), ECodexImplementationState::Implemented,
        {}, { TEXT("shape.cone") }));

    RegisterEntry(MakeEntry(
        TEXT("shape.cone.height"), TEXT("Cone Height"),
        ECodexCategory::ShapeParameter, TEXT("h^"),
        TEXT("Controls the cone height."),
        TEXT("SpellCreation"), ECodexImplementationState::Implemented,
        {}, { TEXT("shape.cone") }));

    // ---------------------------------------------------------------------
    // TIER II — MATERIAL PROPERTIES
    // ---------------------------------------------------------------------
    RegisterEntry(MakeEntry(
        TEXT("material.density"), TEXT("Density"),
        ECodexCategory::MaterialProperty, TEXT("D"),
        TEXT("Mass per unit volume. For Earth constructs this contributes directly to derived mass. Magnitude signs later select a level inside the valid density range."),
        TEXT("MaterialCore"), ECodexImplementationState::Implemented,
        { TEXT("physics.mass_input") },
        { TEXT("material.solid_properties") }));

    RegisterEntry(MakeEntry(
        TEXT("material.hardness"), TEXT("Hardness"),
        ECodexCategory::MaterialProperty, TEXT("H"),
        TEXT("Resistance to local surface deformation or penetration."),
        TEXT("MaterialCore"), ECodexImplementationState::Implemented,
        {}, { TEXT("material.solid_properties") }));

    RegisterEntry(MakeEntry(
        TEXT("material.toughness"), TEXT("Toughness"),
        ECodexCategory::MaterialProperty, TEXT("T"),
        TEXT("Resistance to fracture and destructive failure."),
        TEXT("MaterialCore"), ECodexImplementationState::Implemented,
        {}, { TEXT("material.solid_properties") }));

    RegisterEntry(MakeEntry(
        TEXT("material.elasticity"), TEXT("Elasticity"),
        ECodexCategory::MaterialProperty, TEXT("EL"),
        TEXT("How strongly a material rebounds rather than remaining deformed after contact."),
        TEXT("MaterialCore"), ECodexImplementationState::Implemented,
        {}, { TEXT("material.solid_properties") }));

    // ---------------------------------------------------------------------
    // TIER II — SPATIAL
    // ---------------------------------------------------------------------
    RegisterEntry(MakeEntry(
        TEXT("spatial.distance"), TEXT("Distance"),
        ECodexCategory::Spatial, TEXT("DIST"),
        TEXT("Additional placement distance from the spell safe origin in front of the caster."),
        TEXT("SpellCreation"), ECodexImplementationState::Implemented,
        {}, { TEXT("construct.placeable") }));

    RegisterEntry(MakeEntry(
        TEXT("spatial.orientation"), TEXT("Orientation"),
        ECodexCategory::Spatial, TEXT("ORI"),
        TEXT("Which way a constructed shape faces. Orientation is independent from movement direction."),
        TEXT("SpellCreation"), ECodexImplementationState::Implemented,
        {}, { TEXT("construct.orientable") }));

    RegisterEntry(MakeEntry(
        TEXT("spatial.direction"), TEXT("Direction"),
        ECodexCategory::Spatial, TEXT("->"),
        TEXT("General spatial direction concept. Motion Direction uses the same directional language but applies it specifically to travel."),
        TEXT("WorldCodex"), ECodexImplementationState::CodexOnly,
        { TEXT("reference.direction") }));

    // ---------------------------------------------------------------------
    // TIER III — ORIENTATION / AXIS VALUES USED BY RUNE CANVAS V1
    // ---------------------------------------------------------------------
    RegisterEntry(MakeEntry(
        TEXT("orientation.forward"), TEXT("Face Forward"),
        ECodexCategory::Value, TEXT("OF"),
        TEXT("Terminal Orientation value: face along the spell cast frame Forward axis."),
        TEXT("SpellCreation"), ECodexImplementationState::Implemented,
        { TEXT("value.orientation") }, {},
        TEXT("Attach beneath spatial.orientation."),
        ECodexTier::TierIII));

    RegisterEntry(MakeEntry(
        TEXT("orientation.right"), TEXT("Face Right"),
        ECodexCategory::Value, TEXT("OR"),
        TEXT("Terminal Orientation value: face along the spell cast frame Right axis."),
        TEXT("SpellCreation"), ECodexImplementationState::Implemented,
        { TEXT("value.orientation") }, {},
        TEXT("Attach beneath spatial.orientation."),
        ECodexTier::TierIII));

    RegisterEntry(MakeEntry(
        TEXT("orientation.up"), TEXT("Face Up"),
        ECodexCategory::Value, TEXT("OU"),
        TEXT("Terminal Orientation value: face along the world/cast Up axis."),
        TEXT("SpellCreation"), ECodexImplementationState::Implemented,
        { TEXT("value.orientation") }, {},
        TEXT("Attach beneath spatial.orientation."),
        ECodexTier::TierIII));

    RegisterEntry(MakeEntry(
        TEXT("axis.forward"), TEXT("Axis Forward"),
        ECodexCategory::Value, TEXT("AF"),
        TEXT("Terminal Line Axis value: distribute instances along Forward."),
        TEXT("SpellPattern"), ECodexImplementationState::Implemented,
        { TEXT("value.axis") }, {},
        TEXT("Attach beneath pattern.line_axis."),
        ECodexTier::TierIII));

    RegisterEntry(MakeEntry(
        TEXT("axis.right"), TEXT("Axis Right"),
        ECodexCategory::Value, TEXT("AR"),
        TEXT("Terminal Line Axis value: distribute instances along Right."),
        TEXT("SpellPattern"), ECodexImplementationState::Implemented,
        { TEXT("value.axis") }, {},
        TEXT("Attach beneath pattern.line_axis."),
        ECodexTier::TierIII));

    RegisterEntry(MakeEntry(
        TEXT("axis.up"), TEXT("Axis Up"),
        ECodexCategory::Value, TEXT("AU"),
        TEXT("Terminal Line Axis value: distribute instances vertically."),
        TEXT("SpellPattern"), ECodexImplementationState::Implemented,
        { TEXT("value.axis") }, {},
        TEXT("Attach beneath pattern.line_axis."),
        ECodexTier::TierIII));

    // ---------------------------------------------------------------------
    // TIER II — PATTERNS
    // ---------------------------------------------------------------------
    RegisterEntry(MakeEntry(
        TEXT("pattern.amount"), TEXT("Amount"),
        ECodexCategory::Pattern, TEXT("#"),
        TEXT("Number of realized instances in the current spell pattern."),
        TEXT("SpellPattern"), ECodexImplementationState::Implemented,
        {}, { TEXT("construct.instanceable") }));

    RegisterEntry(MakeEntry(
        TEXT("pattern.line"), TEXT("Line"),
        ECodexCategory::Pattern, TEXT("---"),
        TEXT("Arranges multiple instances along a cast-relative axis."),
        TEXT("SpellPattern"), ECodexImplementationState::Implemented,
        { TEXT("pattern.line") },
        { TEXT("construct.instanceable") }));

    RegisterEntry(MakeEntry(
        TEXT("pattern.circle"), TEXT("Circle"),
        ECodexCategory::Pattern, TEXT("(O)"),
        TEXT("Arranges multiple instances around a common horizontal center."),
        TEXT("SpellPattern"), ECodexImplementationState::Implemented,
        { TEXT("pattern.circle") },
        { TEXT("construct.instanceable") }));

    RegisterEntry(MakeEntry(
        TEXT("pattern.line_axis"), TEXT("Line Axis"),
        ECodexCategory::Pattern, TEXT("AX"),
        TEXT("Selects the Forward, Right or Up axis used by a line pattern."),
        TEXT("SpellPattern"), ECodexImplementationState::Implemented,
        {}, { TEXT("pattern.line") }));

    RegisterEntry(MakeEntry(
        TEXT("pattern.spacing"), TEXT("Spacing"),
        ECodexCategory::Pattern, TEXT("<->"),
        TEXT("Center-to-center spacing between instances in a line."),
        TEXT("SpellPattern"), ECodexImplementationState::Implemented,
        {}, { TEXT("pattern.line") }));

    RegisterEntry(MakeEntry(
        TEXT("pattern.circle_radius"), TEXT("Circle Radius"),
        ECodexCategory::Pattern, TEXT("R(O)"),
        TEXT("Radius of the circular instance arrangement."),
        TEXT("SpellPattern"), ECodexImplementationState::Implemented,
        {}, { TEXT("pattern.circle") }));

    RegisterEntry(MakeEntry(
        TEXT("pattern.orientation"), TEXT("Instance Orientation"),
        ECodexCategory::Pattern, TEXT("FACE"),
        TEXT("Controls each instance facing relative to its pattern. It never decides movement direction."),
        TEXT("SpellPattern"), ECodexImplementationState::Implemented,
        {}, { TEXT("construct.orientable"), TEXT("construct.instanceable") }));

    RegisterEntry(MakeEntry(
        TEXT("pattern.orientation.shared"), TEXT("Shared Orientation"),
        ECodexCategory::Pattern, TEXT("="),
        TEXT("All pattern instances retain the same base spell orientation."),
        TEXT("SpellPattern"), ECodexImplementationState::Implemented,
        {}, {}, TEXT("Terminal value for Instance Orientation."), ECodexTier::TierIII));

    RegisterEntry(MakeEntry(
        TEXT("pattern.orientation.outward"), TEXT("Outward Orientation"),
        ECodexCategory::Pattern, TEXT("><"),
        TEXT("Each instance faces away from the pattern or line center."),
        TEXT("SpellPattern"), ECodexImplementationState::Implemented,
        {}, {}, TEXT("Terminal value for Instance Orientation. Facing only."), ECodexTier::TierIII));

    RegisterEntry(MakeEntry(
        TEXT("pattern.orientation.inward"), TEXT("Inward Orientation"),
        ECodexCategory::Pattern, TEXT("<>"),
        TEXT("Each instance faces toward the pattern or line center."),
        TEXT("SpellPattern"), ECodexImplementationState::Implemented,
        {}, {}, TEXT("Terminal value for Instance Orientation. Facing only."), ECodexTier::TierIII));

    RegisterEntry(MakeEntry(
        TEXT("pattern.orientation.tangent"), TEXT("Tangent Orientation"),
        ECodexCategory::Pattern, TEXT("~>"),
        TEXT("Each instance faces along the tangent or line-axis direction of its pattern."),
        TEXT("SpellPattern"), ECodexImplementationState::Implemented,
        {}, {}, TEXT("Terminal value for Instance Orientation. Facing only."), ECodexTier::TierIII));

    // ---------------------------------------------------------------------
    // TIER II — MOTION
    // ---------------------------------------------------------------------
    RegisterEntry(MakeEntry(
        TEXT("motion.speed"), TEXT("Speed"),
        ECodexCategory::Motion, TEXT(">>"),
        TEXT("Initial linear speed magnitude. Speed decides how fast an object travels; Motion Direction decides where it travels."),
        TEXT("SpellExecution"), ECodexImplementationState::Implemented,
        { TEXT("motion.velocity") },
        { TEXT("construct.movable") },
        TEXT("Current slider remains numeric. Magnitude signs are the future Rune Canvas language.")));

    RegisterEntry(MakeEntry(
        TEXT("motion.direction"), TEXT("Motion Direction"),
        ECodexCategory::Motion, TEXT("DIR"),
        TEXT("Independent movement direction for each realized spell instance. It is intentionally separate from shape and pattern orientation."),
        TEXT("SpellMotion"), ECodexImplementationState::Implemented,
        { TEXT("motion.direction") },
        { TEXT("construct.movable") }));

    // Motion Direction Tier III values.
    RegisterEntry(MakeEntry(
        TEXT("direction.forward"), TEXT("Forward"),
        ECodexCategory::Value, TEXT("FWD"),
        TEXT("Travel along the current cast/aim direction."),
        TEXT("SpellMotion"), ECodexImplementationState::Implemented,
        { TEXT("value.direction") }, {}, TEXT("Motion-direction terminal value."), ECodexTier::TierIII));

    RegisterEntry(MakeEntry(
        TEXT("direction.backward"), TEXT("Backward"),
        ECodexCategory::Value, TEXT("BACK"),
        TEXT("Travel opposite the current cast/aim direction."),
        TEXT("SpellMotion"), ECodexImplementationState::Implemented,
        { TEXT("value.direction") }, {}, TEXT("Motion-direction terminal value."), ECodexTier::TierIII));

    RegisterEntry(MakeEntry(
        TEXT("direction.up"), TEXT("Up"),
        ECodexCategory::Value, TEXT("UP"),
        TEXT("Travel upward in world space."),
        TEXT("SpellMotion"), ECodexImplementationState::Implemented,
        { TEXT("value.direction") }, {}, TEXT("Motion-direction terminal value."), ECodexTier::TierIII));

    RegisterEntry(MakeEntry(
        TEXT("direction.down"), TEXT("Down"),
        ECodexCategory::Value, TEXT("DOWN"),
        TEXT("Travel downward in world space."),
        TEXT("SpellMotion"), ECodexImplementationState::Implemented,
        { TEXT("value.direction") }, {}, TEXT("Motion-direction terminal value."), ECodexTier::TierIII));

    RegisterEntry(MakeEntry(
        TEXT("direction.outward"), TEXT("Outward"),
        ECodexCategory::Value, TEXT("OUT"),
        TEXT("Each instance travels away from the pattern center. A single centered instance falls back to Forward."),
        TEXT("SpellMotion"), ECodexImplementationState::Implemented,
        { TEXT("value.direction") }, {}, TEXT("Pattern-relative motion terminal value."), ECodexTier::TierIII));

    RegisterEntry(MakeEntry(
        TEXT("direction.inward"), TEXT("Inward"),
        ECodexCategory::Value, TEXT("IN"),
        TEXT("Each instance travels toward the pattern center. A single centered instance falls back to Forward."),
        TEXT("SpellMotion"), ECodexImplementationState::Implemented,
        { TEXT("value.direction") }, {}, TEXT("Pattern-relative motion terminal value."), ECodexTier::TierIII));

    RegisterEntry(MakeEntry(
        TEXT("direction.tangent"), TEXT("Tangent"),
        ECodexCategory::Value, TEXT("TAN"),
        TEXT("Circle instances launch along the local tangent to the circumference; line instances launch along the selected line axis. This is an initial direction, not orbital motion."),
        TEXT("SpellMotion"), ECodexImplementationState::Implemented,
        { TEXT("value.direction") }, {}, TEXT("Pattern-relative motion terminal value."), ECodexTier::TierIII));

    // ---------------------------------------------------------------------
    // ACTIONS
    // ---------------------------------------------------------------------
    RegisterEntry(MakeEntry(
        TEXT("action.create"), TEXT("Create"),
        ECodexCategory::Action, TEXT("+"),
        TEXT("Realize a valid magical construction into the world."),
        TEXT("SpellExecution"), ECodexImplementationState::Implemented,
        { TEXT("action.create") }));

    RegisterEntry(MakeEntry(
        TEXT("action.launch"), TEXT("Launch"),
        ECodexCategory::Action, TEXT("=>"),
        TEXT("Give a realized movable construct an initial velocity."),
        TEXT("EarthMagic"), ECodexImplementationState::Implemented,
        { TEXT("action.launch") },
        { TEXT("construct.movable"), TEXT("motion.velocity") }));

    // ---------------------------------------------------------------------
    // WORLD REFERENCES / STATES
    // ---------------------------------------------------------------------
    RegisterEntry(MakeEntry(
        TEXT("world.person"), TEXT("Person"),
        ECodexCategory::WorldObject, TEXT("P"),
        TEXT("A person or character reference in the world."),
        TEXT("WorldCodex"), ECodexImplementationState::CodexOnly,
        { TEXT("reference.world_object"), TEXT("reference.person") }));

    RegisterEntry(MakeEntry(
        TEXT("world.ground"), TEXT("Ground"),
        ECodexCategory::WorldObject, TEXT("_"),
        TEXT("A supporting world surface reference."),
        TEXT("WorldCodex"), ECodexImplementationState::CodexOnly,
        { TEXT("reference.world_object"), TEXT("reference.surface") }));

    RegisterEntry(MakeEntry(
        TEXT("world.state.airborne"), TEXT("Airborne"),
        ECodexCategory::WorldState, TEXT("AIR?"),
        TEXT("Boolean state of a referenced physical object. True while that object has no valid supporting ground/surface contact. This is a state predicate, not an event and not a repeat instruction."),
        TEXT("WorldCodex"), ECodexImplementationState::CodexOnly,
        { TEXT("value.boolean") },
        { TEXT("reference.world_object") },
        TEXT("Use IF with this state for a one-time condition, WHILE for sustained behavior, and a future Became Airborne event with WHEN for the transition itself.")));

    // ---------------------------------------------------------------------
    // EVENTS
    // ---------------------------------------------------------------------
    RegisterEntry(MakeEntry(
        TEXT("event.impact"), TEXT("Impact"),
        ECodexCategory::Event, TEXT("*"),
        TEXT("Collision/impact event. Later logic can reference impact position, surface normal, hit object and impact force."),
        TEXT("ImpactSystem"), ECodexImplementationState::CodexOnly,
        {
            TEXT("event"),
            TEXT("event.position"),
            TEXT("event.normal"),
            TEXT("event.hit_object"),
            TEXT("event.force")
        }));

    RegisterEntry(MakeEntry(
        TEXT("event.became_airborne"), TEXT("Became Airborne"),
        ECodexCategory::Event, TEXT("AIR^"),
        TEXT("Future transition event emitted when a referenced physical object changes from supported/grounded to airborne."),
        TEXT("WorldCodex"), ECodexImplementationState::Planned,
        { TEXT("event") },
        { TEXT("reference.world_object") }));

    RegisterEntry(MakeEntry(
        TEXT("event.landed"), TEXT("Landed"),
        ECodexCategory::Event, TEXT("LAND"),
        TEXT("Future transition event emitted when an airborne physical object gains valid supporting contact."),
        TEXT("WorldCodex"), ECodexImplementationState::Planned,
        { TEXT("event") },
        { TEXT("reference.world_object") }));

    // ---------------------------------------------------------------------
    // LOGIC — documented now, executable later.
    // ---------------------------------------------------------------------
    RegisterEntry(MakeEntry(
        TEXT("logic.when"), TEXT("When"),
        ECodexCategory::Logic, TEXT("WHEN"),
        TEXT("Event trigger. Executes a connected construction when a discrete event occurs; it should consume events such as Impact or Became Airborne rather than continuous states."),
        TEXT("WorldCodex"), ECodexImplementationState::Planned,
        { TEXT("logic.trigger") },
        { TEXT("event") }));

    RegisterEntry(MakeEntry(
        TEXT("logic.if"), TEXT("If"),
        ECodexCategory::Logic, TEXT("IF"),
        TEXT("One-time conditional branch based on a boolean state or comparison."),
        TEXT("WorldCodex"), ECodexImplementationState::Planned,
        { TEXT("logic.branch") },
        { TEXT("value.boolean") }));

    RegisterEntry(MakeEntry(
        TEXT("logic.repeat"), TEXT("Repeat"),
        ECodexCategory::Logic, TEXT("xN"),
        TEXT("Bounded repetition of a connected construction or action."),
        TEXT("WorldCodex"), ECodexImplementationState::Planned,
        { TEXT("logic.repeat") }));

    RegisterEntry(MakeEntry(
        TEXT("logic.while"), TEXT("While"),
        ECodexCategory::Logic, TEXT("WHILE"),
        TEXT("Persistent conditional repetition while a boolean state remains true. Planned with execution and mana safety limits."),
        TEXT("WorldCodex"), ECodexImplementationState::Planned,
        { TEXT("logic.loop") },
        { TEXT("value.boolean") }));

    // ---------------------------------------------------------------------
    // TIER III — UNIVERSAL MAGNITUDE LANGUAGE
    // 0..5 is a normalized vocabulary. The parent concept decides the exact
    // physical mapping. Example: Speed 5 = max speed; Density 0 maps to its
    // minimum valid density rather than impossible zero-density Earth.
    // ---------------------------------------------------------------------
    RegisterEntry(MakeEntry(
        TEXT("value.magnitude.0"), TEXT("Magnitude 0 / None"),
        ECodexCategory::Value, TEXT("0"),
        TEXT("Lowest magnitude. Means zero/none when that parent concept allows zero; otherwise maps to the parent's minimum valid state."),
        TEXT("WorldCodex"), ECodexImplementationState::CodexOnly,
        { TEXT("value.magnitude"), TEXT("value.magnitude.0") },
        {}, TEXT("Normalized level 0 of 5."), ECodexTier::TierIII));

    RegisterEntry(MakeEntry(
        TEXT("value.magnitude.1"), TEXT("Magnitude 1 / Very Low"),
        ECodexCategory::Value, TEXT("I"),
        TEXT("Very-low normalized magnitude. Exact physical value is defined by the parent modifier."),
        TEXT("WorldCodex"), ECodexImplementationState::CodexOnly,
        { TEXT("value.magnitude"), TEXT("value.magnitude.1") },
        {}, TEXT("Normalized level 1 of 5."), ECodexTier::TierIII));

    RegisterEntry(MakeEntry(
        TEXT("value.magnitude.2"), TEXT("Magnitude 2 / Low"),
        ECodexCategory::Value, TEXT("II"),
        TEXT("Low normalized magnitude. Exact physical value is defined by the parent modifier."),
        TEXT("WorldCodex"), ECodexImplementationState::CodexOnly,
        { TEXT("value.magnitude"), TEXT("value.magnitude.2") },
        {}, TEXT("Normalized level 2 of 5."), ECodexTier::TierIII));

    RegisterEntry(MakeEntry(
        TEXT("value.magnitude.3"), TEXT("Magnitude 3 / Medium"),
        ECodexCategory::Value, TEXT("III"),
        TEXT("Medium normalized magnitude. Exact physical value is defined by the parent modifier."),
        TEXT("WorldCodex"), ECodexImplementationState::CodexOnly,
        { TEXT("value.magnitude"), TEXT("value.magnitude.3") },
        {}, TEXT("Normalized level 3 of 5."), ECodexTier::TierIII));

    RegisterEntry(MakeEntry(
        TEXT("value.magnitude.4"), TEXT("Magnitude 4 / High"),
        ECodexCategory::Value, TEXT("IV"),
        TEXT("High normalized magnitude. Exact physical value is defined by the parent modifier."),
        TEXT("WorldCodex"), ECodexImplementationState::CodexOnly,
        { TEXT("value.magnitude"), TEXT("value.magnitude.4") },
        {}, TEXT("Normalized level 4 of 5."), ECodexTier::TierIII));

    RegisterEntry(MakeEntry(
        TEXT("value.magnitude.5"), TEXT("Magnitude 5 / Maximum"),
        ECodexCategory::Value, TEXT("V"),
        TEXT("Maximum normalized magnitude for the parent modifier's currently allowed range."),
        TEXT("WorldCodex"), ECodexImplementationState::CodexOnly,
        { TEXT("value.magnitude"), TEXT("value.magnitude.5") },
        {}, TEXT("Normalized level 5 of 5."), ECodexTier::TierIII));

    // ---------------------------------------------------------------------
    // WORLD PHYSICS
    // ---------------------------------------------------------------------
    RegisterEntry(MakeEntry(
        TEXT("physics.mass"), TEXT("Mass"),
        ECodexCategory::PhysicsConcept, TEXT("M"),
        TEXT("Derived physical mass. Current Earth mass is determined by volume and density."),
        TEXT("PhysicalBody"), ECodexImplementationState::Implemented,
        { TEXT("physics.mass") },
        { TEXT("physics.rigid_body"), TEXT("physics.mass_input") },
        TEXT("Mass is currently derived, not directly authored by the Earth spell editor.")));

    RegisterEntry(MakeEntry(
        TEXT("physics.gravity"), TEXT("Gravity"),
        ECodexCategory::PhysicsConcept, TEXT("G"),
        TEXT("World force that accelerates unsupported physical bodies downward."),
        TEXT("PhysicalBody"), ECodexImplementationState::Implemented));

    RegisterEntry(MakeEntry(
        TEXT("physics.collision"), TEXT("Collision"),
        ECodexCategory::PhysicsConcept, TEXT("COL"),
        TEXT("Physical contact between bodies or world geometry."),
        TEXT("ImpactSystem"), ECodexImplementationState::Implemented,
        { TEXT("event.collision") }));

    RegisterEntry(MakeEntry(
        TEXT("physics.momentum"), TEXT("Momentum"),
        ECodexCategory::PhysicsConcept, TEXT("MV"),
        TEXT("Motion quantity derived from mass and velocity. Present in the Codex for later world-interaction expansion."),
        TEXT("ImpactSystem"), ECodexImplementationState::CodexOnly,
        { TEXT("physics.momentum") },
        { TEXT("physics.mass"), TEXT("motion.velocity") }));
}
