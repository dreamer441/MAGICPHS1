#include "SpellCastPlacement.h"

#include "SpellShapeMath.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

bool FSpellCastPlacement::Resolve(
    APlayerController* PlayerController,
    const FResolvedSpell& Spell,
    const FVector& DesiredAimDirection,
    FResolvedSpellCastPlacement& OutPlacement)
{
    APawn* Pawn = PlayerController ? PlayerController->GetPawn() : nullptr;
    if (!Pawn)
    {
        return false;
    }

    FVector LaunchDirection = DesiredAimDirection.GetSafeNormal();
    if (LaunchDirection.IsNearlyZero())
    {
        LaunchDirection = Pawn->GetActorForwardVector().GetSafeNormal();
    }

    // Spawn position is character-relative, never camera-relative. Use a horizontal
    // forward direction for placement so an over-shoulder camera cannot create the
    // spell behind or below the character; launch direction can still retain pitch.
    FVector PlacementForward(LaunchDirection.X, LaunchDirection.Y, 0.0f);
    if (!PlacementForward.Normalize())
    {
        PlacementForward = Pawn->GetActorForwardVector();
        PlacementForward.Z = 0.0f;
        PlacementForward.Normalize();
    }

    const FSpellDefinition& Definition = Spell.Definition;
    const FVector HalfExtents = FSpellShapeMath::CalculateHalfExtentsCm(Definition);
    const float ShapeClearanceCm = FMath::Max(HalfExtents.GetMax(), 20.0f);
    constexpr float CharacterSafetyGapCm = 110.0f;
    constexpr float ChestOffsetCm = 70.0f;
    const float ExtraDistanceCm = FMath::Max(Definition.DistanceM, 0.0f) * 100.0f;

    OutPlacement.SpawnLocation = Pawn->GetActorLocation()
        + FVector(0.0f, 0.0f, ChestOffsetCm)
        + PlacementForward * (CharacterSafetyGapCm + ShapeClearanceCm + ExtraDistanceCm);
    OutPlacement.LaunchDirection = LaunchDirection;

    // Shape orientation is independent from aim/launch direction.
    switch (Definition.Orientation)
    {
    case ESpellOrientationAxis::Forward:
        OutPlacement.SpawnRotation = FRotationMatrix::MakeFromZX(
            LaunchDirection,
            FVector::UpVector).Rotator();
        break;

    case ESpellOrientationAxis::Right:
    {
        FVector CastRight = FVector::CrossProduct(FVector::UpVector, PlacementForward).GetSafeNormal();
        if (CastRight.IsNearlyZero())
        {
            CastRight = Pawn->GetActorRightVector().GetSafeNormal();
        }
        OutPlacement.SpawnRotation = FRotationMatrix::MakeFromZX(
            CastRight,
            FVector::UpVector).Rotator();
        break;
    }

    case ESpellOrientationAxis::Up:
    default:
        OutPlacement.SpawnRotation = FRotator(0.0f, PlacementForward.Rotation().Yaw, 0.0f);
        break;
    }

    return true;
}
