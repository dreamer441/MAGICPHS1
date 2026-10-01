#include "SpellCastingBindingSubsystem.h"

#include "LiveSpellSessionSubsystem.h"
#include "LiveSpellTuning.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

void USpellCastingBindingSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    // Playable defaults. A is deliberately shared across the primary dimension
    // of each shape; only the currently resolved shape consumes that action.
    Bindings.Add(ESpellLiveAction::SelectEarth, EKeys::Q);
    Bindings.Add(ESpellLiveAction::SphereRadius, EKeys::A);
    Bindings.Add(ESpellLiveAction::CubeX, EKeys::A);
    Bindings.Add(ESpellLiveAction::ConeRadius, EKeys::A);
    Bindings.Add(ESpellLiveAction::Speed, EKeys::S);
    Bindings.Add(ESpellLiveAction::Density, EKeys::D);
}

void USpellCastingBindingSubsystem::Deinitialize()
{
    ResetActiveHolds();
    KeysWaitingForRelease.Empty();
    Bindings.Empty();
    Super::Deinitialize();
}

TStatId USpellCastingBindingSubsystem::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(USpellCastingBindingSubsystem, STATGROUP_Tickables);
}

void USpellCastingBindingSubsystem::SetTopDownLiveInputEnabled(const bool bEnabled)
{
    if (bTopDownLiveInputEnabled == bEnabled)
    {
        return;
    }
    bTopDownLiveInputEnabled = bEnabled;
    ResetActiveHolds();
}

void USpellCastingBindingSubsystem::SetSuspended(const bool bInSuspended)
{
    if (bSuspended == bInSuspended)
    {
        return;
    }
    bSuspended = bInSuspended;
    ResetActiveHolds();
}

void USpellCastingBindingSubsystem::ResetActiveHolds()
{
    ActiveHoldSeconds.Empty();
}

FKey USpellCastingBindingSubsystem::GetBinding(const ESpellLiveAction Action) const
{
    if (const FKey* Found = Bindings.Find(Action))
    {
        return *Found;
    }
    return EKeys::Invalid;
}

FText USpellCastingBindingSubsystem::GetBindingLabel(const ESpellLiveAction Action) const
{
    const FKey Key = GetBinding(Action);
    return Key.IsValid() ? Key.GetDisplayName() : FText::FromString(TEXT("+"));
}

bool USpellCastingBindingSubsystem::CanAssignKey(const FKey& Key) const
{
    if (!Key.IsValid() || Key.IsMouseButton() || Key.IsGamepadKey())
    {
        return false;
    }

    return Key != EKeys::Tab
        && Key != EKeys::LeftShift
        && Key != EKeys::RightShift
        && Key != EKeys::SpaceBar
        && Key != EKeys::Escape
        && Key != EKeys::One
        && Key != EKeys::Two
        && Key != EKeys::Three
        && Key != EKeys::Four
        && Key != EKeys::Five
        && Key != EKeys::Six
        && Key != EKeys::Seven
        && Key != EKeys::Eight
        && Key != EKeys::Nine
        && Key != EKeys::Zero
        && Key != EKeys::MouseScrollUp
        && Key != EKeys::MouseScrollDown
        && Key != EKeys::MouseWheelAxis;
}

bool USpellCastingBindingSubsystem::IsShapeSelector(const ESpellLiveAction Action) const
{
    return Action == ESpellLiveAction::SelectSphere
        || Action == ESpellLiveAction::SelectCube
        || Action == ESpellLiveAction::SelectCone;
}

bool USpellCastingBindingSubsystem::SetBinding(const ESpellLiveAction Action, const FKey& Key)
{
    if (!CanAssignKey(Key))
    {
        return false;
    }

    // Selecting the same key again means "unassign".
    if (GetBinding(Action) == Key)
    {
        ClearBinding(Action);
        return true;
    }

    // Compatible value actions may deliberately share a key (for example Cube
    // X/Y/Z all on A). Shape selectors remain mutually exclusive with each other
    // so one tap cannot choose three conflicting shapes at once.
    if (IsShapeSelector(Action))
    {
        TArray<ESpellLiveAction> ToClear;
        for (const TPair<ESpellLiveAction, FKey>& Pair : Bindings)
        {
            if (Pair.Key != Action && IsShapeSelector(Pair.Key) && Pair.Value == Key)
            {
                ToClear.Add(Pair.Key);
            }
        }
        for (const ESpellLiveAction Other : ToClear)
        {
            Bindings.Remove(Other);
            ActiveHoldSeconds.Remove(Other);
        }
    }

    Bindings.Add(Action, Key);
    ActiveHoldSeconds.Remove(Action);
    KeysWaitingForRelease.Remove(Key);
    return true;
}

void USpellCastingBindingSubsystem::ClearBinding(const ESpellLiveAction Action)
{
    const FKey Existing = GetBinding(Action);
    Bindings.Remove(Action);
    ActiveHoldSeconds.Remove(Action);
    if (Existing.IsValid())
    {
        KeysWaitingForRelease.Remove(Existing);
    }
}

bool USpellCastingBindingSubsystem::IsContinuousAction(const ESpellLiveAction Action) const
{
    switch (Action)
    {
        case ESpellLiveAction::SphereRadius:
        case ESpellLiveAction::CubeX:
        case ESpellLiveAction::CubeY:
        case ESpellLiveAction::CubeZ:
        case ESpellLiveAction::ConeRadius:
        case ESpellLiveAction::ConeHeight:
        case ESpellLiveAction::Speed:
        case ESpellLiveAction::Density:
        case ESpellLiveAction::Distance:
        case ESpellLiveAction::Hardness:
        case ESpellLiveAction::Toughness:
        case ESpellLiveAction::Elasticity:
            return true;
        default:
            return false;
    }
}

float USpellCastingBindingSubsystem::GetLiveValue01(const ESpellLiveAction Action) const
{
    UWorld* World = GetWorld();
    const ULiveSpellSessionSubsystem* Session = World ? World->GetSubsystem<ULiveSpellSessionSubsystem>() : nullptr;
    if (!Session)
    {
        return 0.0f;
    }

    ELiveSpellParameter Parameter;
    switch (Action)
    {
        case ESpellLiveAction::SphereRadius: Parameter = ELiveSpellParameter::SphereRadius; break;
        case ESpellLiveAction::CubeX: Parameter = ELiveSpellParameter::CubeX; break;
        case ESpellLiveAction::CubeY: Parameter = ELiveSpellParameter::CubeY; break;
        case ESpellLiveAction::CubeZ: Parameter = ELiveSpellParameter::CubeZ; break;
        case ESpellLiveAction::ConeRadius: Parameter = ELiveSpellParameter::ConeRadius; break;
        case ESpellLiveAction::ConeHeight: Parameter = ELiveSpellParameter::ConeHeight; break;
        case ESpellLiveAction::Speed: Parameter = ELiveSpellParameter::Speed; break;
        case ESpellLiveAction::Density: Parameter = ELiveSpellParameter::Density; break;
        case ESpellLiveAction::Distance: Parameter = ELiveSpellParameter::Distance; break;
        case ESpellLiveAction::Hardness: Parameter = ELiveSpellParameter::Hardness; break;
        case ESpellLiveAction::Toughness: Parameter = ELiveSpellParameter::Toughness; break;
        case ESpellLiveAction::Elasticity: Parameter = ELiveSpellParameter::Elasticity; break;
        default: return 0.0f;
    }
    return Session->GetParameterNormalized(Parameter);
}

void USpellCastingBindingSubsystem::ApplyToggleAction(const ESpellLiveAction Action)
{
    UWorld* World = GetWorld();
    ULiveSpellSessionSubsystem* Session = World ? World->GetSubsystem<ULiveSpellSessionSubsystem>() : nullptr;
    if (!Session)
    {
        return;
    }

    const TCHAR* DebugText = TEXT("");
    switch (Action)
    {
        case ESpellLiveAction::SelectEarth:
            // Element is always the root action. Selecting it starts/restarts a spell.
            Session->SelectEarth();
            DebugText = TEXT("ELEMENT: EARTH");
            break;

        case ESpellLiveAction::SelectSphere:
        case ESpellLiveAction::SelectCube:
        case ESpellLiveAction::SelectCone:
        {
            // Hierarchy gate: shape keys are inert until an element exists.
            if (!Session->CanSelectShape())
            {
                return;
            }

            if (Action == ESpellLiveAction::SelectSphere)
            {
                Session->SelectShape(EEarthSpellShape::Sphere);
                DebugText = TEXT("SHAPE: SPHERE");
            }
            else if (Action == ESpellLiveAction::SelectCube)
            {
                Session->SelectShape(EEarthSpellShape::Cube);
                DebugText = TEXT("SHAPE: CUBE");
            }
            else
            {
                Session->SelectShape(EEarthSpellShape::Cone);
                DebugText = TEXT("SHAPE: CONE");
            }
            break;
        }

        default:
            return;
    }

    if (GEngine && DebugText[0] != 0)
    {
        GEngine->AddOnScreenDebugMessage(-1, 0.6f, FColor::Green, DebugText);
    }
}

void USpellCastingBindingSubsystem::ApplyContinuousValue(const ESpellLiveAction Action, const float NormalizedValue)
{
    UWorld* World = GetWorld();
    ULiveSpellSessionSubsystem* Session = World ? World->GetSubsystem<ULiveSpellSessionSubsystem>() : nullptr;
    if (!Session)
    {
        return;
    }

    ELiveSpellParameter Parameter;
    switch (Action)
    {
        case ESpellLiveAction::SphereRadius: Parameter = ELiveSpellParameter::SphereRadius; break;
        case ESpellLiveAction::CubeX: Parameter = ELiveSpellParameter::CubeX; break;
        case ESpellLiveAction::CubeY: Parameter = ELiveSpellParameter::CubeY; break;
        case ESpellLiveAction::CubeZ: Parameter = ELiveSpellParameter::CubeZ; break;
        case ESpellLiveAction::ConeRadius: Parameter = ELiveSpellParameter::ConeRadius; break;
        case ESpellLiveAction::ConeHeight: Parameter = ELiveSpellParameter::ConeHeight; break;
        case ESpellLiveAction::Speed: Parameter = ELiveSpellParameter::Speed; break;
        case ESpellLiveAction::Density: Parameter = ELiveSpellParameter::Density; break;
        case ESpellLiveAction::Distance: Parameter = ELiveSpellParameter::Distance; break;
        case ESpellLiveAction::Hardness: Parameter = ELiveSpellParameter::Hardness; break;
        case ESpellLiveAction::Toughness: Parameter = ELiveSpellParameter::Toughness; break;
        case ESpellLiveAction::Elasticity: Parameter = ELiveSpellParameter::Elasticity; break;
        default: return;
    }

    Session->SetParameterNormalized(Parameter, NormalizedValue);
}

bool USpellCastingBindingSubsystem::IsActionApplicableToShape(const ESpellLiveAction Action) const
{
    UWorld* World = GetWorld();
    const ULiveSpellSessionSubsystem* Session = World ? World->GetSubsystem<ULiveSpellSessionSubsystem>() : nullptr;

    // Hierarchy gate: no modifier key has meaning until the player has explicitly
    // chosen both an element and a shape in the current live construction.
    if (!Session || !Session->CanApplyModifier())
    {
        return false;
    }

    const EEarthSpellShape Shape = Session->GetResolvedShape();

    switch (Action)
    {
        case ESpellLiveAction::SphereRadius:
            return Shape == EEarthSpellShape::Sphere;
        case ESpellLiveAction::CubeX:
        case ESpellLiveAction::CubeY:
        case ESpellLiveAction::CubeZ:
            return Shape == EEarthSpellShape::Cube;
        case ESpellLiveAction::ConeRadius:
        case ESpellLiveAction::ConeHeight:
            return Shape == EEarthSpellShape::Cone;
        default:
            return true;
    }
}

void USpellCastingBindingSubsystem::Tick(const float DeltaSeconds)
{
    if (!bTopDownLiveInputEnabled || bSuspended)
    {
        return;
    }

    UWorld* World = GetWorld();
    if (!World || !World->IsGameWorld())
    {
        return;
    }

    APlayerController* PC = World->GetFirstPlayerController();
    ULiveSpellSessionSubsystem* Session = World->GetSubsystem<ULiveSpellSessionSubsystem>();
    if (!PC || !Session)
    {
        return;
    }

    // If SPACE consumed/reset the spell while construction keys were still held,
    // do not silently begin a second spell. Those physical keys must be released first.
    if (Session->GetGeneration() != LastObservedSessionGeneration)
    {
        LastObservedSessionGeneration = Session->GetGeneration();
        ResetActiveHolds();
        KeysWaitingForRelease.Empty();
        for (const TPair<ESpellLiveAction, FKey>& Pair : Bindings)
        {
            if (Pair.Value.IsValid() && PC->IsInputKeyDown(Pair.Value))
            {
                KeysWaitingForRelease.Add(Pair.Value);
            }
        }
    }

    TArray<FKey> ReleasedBlockedKeys;
    for (const FKey& Key : KeysWaitingForRelease)
    {
        if (!PC->IsInputKeyDown(Key))
        {
            ReleasedBlockedKeys.Add(Key);
        }
    }
    for (const FKey& Key : ReleasedBlockedKeys)
    {
        KeysWaitingForRelease.Remove(Key);
    }

    // Pass 1: selectors. This makes shape + dimension bindings deterministic even
    // when they intentionally share a key.
    for (const TPair<ESpellLiveAction, FKey>& Pair : Bindings)
    {
        const ESpellLiveAction Action = Pair.Key;
        const FKey Key = Pair.Value;
        if (!Key.IsValid() || KeysWaitingForRelease.Contains(Key) || IsContinuousAction(Action))
        {
            continue;
        }
        if (PC->WasInputKeyJustPressed(Key))
        {
            ApplyToggleAction(Action);
        }
    }

    // Pass 2: held values. Only dimensions belonging to the resolved current shape
    // participate; shared A bindings for other shapes remain dormant.
    for (const TPair<ESpellLiveAction, FKey>& Pair : Bindings)
    {
        const ESpellLiveAction Action = Pair.Key;
        const FKey Key = Pair.Value;
        if (!Key.IsValid() || !IsContinuousAction(Action))
        {
            continue;
        }

        if (KeysWaitingForRelease.Contains(Key) || !IsActionApplicableToShape(Action))
        {
            ActiveHoldSeconds.Remove(Action);
            continue;
        }

        const bool bDown = PC->IsInputKeyDown(Key);
        float* HoldSeconds = ActiveHoldSeconds.Find(Action);

        if (!bDown)
        {
            if (HoldSeconds)
            {
                ActiveHoldSeconds.Remove(Action);
            }
            continue;
        }

        if (!HoldSeconds)
        {
            ActiveHoldSeconds.Add(Action, 0.0f);
            HoldSeconds = ActiveHoldSeconds.Find(Action);
            ApplyContinuousValue(Action, 0.0f);
        }

        *HoldSeconds = FMath::Min(
            *HoldSeconds + FMath::Max(DeltaSeconds, 0.0f),
            LiveSpellTuning::FullChargeSeconds);
        ApplyContinuousValue(Action, LiveSpellTuning::ChargeToValue01(*HoldSeconds));
    }
}
