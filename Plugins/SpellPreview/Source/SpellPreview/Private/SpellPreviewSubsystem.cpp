#include "SpellPreviewSubsystem.h"

#include "DrawDebugHelpers.h"
#include "EarthSpellDefinition.h"
#include "EngineUtils.h"
#include "InnerRealmActor.h"
#include "InnerRealmSubsystem.h"
#include "SpellParameterRanges.h"
#include "LiveSpellSessionSubsystem.h"
#include "PlayerViewModeSubsystem.h"
#include "SpellCastPlacement.h"
#include "SpellDefinition.h"
#include "SpellCreationSubsystem.h"
#include "SpellPatternResolver.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

namespace
{
    constexpr float PreviewThickness = 1.8f;
    constexpr float SpeedRingThickness = 3.0f;

    const FColor EarthGreen(60, 220, 95, 105);
    const FColor SpeedGreen(155, 255, 185, 150);
    const FColor WorkbenchGuide(95, 155, 105, 95);

    int32 GetDensityGridSteps(const FEarthSpellDefinition& Spell)
    {
        const float N = SpellParameterRanges::Normalize(
            Spell.DensityKgPerM3,
            SpellParameterRanges::MinDensityKgPerM3,
            SpellParameterRanges::MaxDensityKgPerM3);

        // Shared visual density language:
        // low density = sparse construction grid, max density = dense grid.
        return FMath::Clamp(FMath::RoundToInt(FMath::Lerp(4.0f, 15.0f, N)), 4, 15);
    }

    int32 GetSpeedRingCount(const FEarthSpellDefinition& Spell)
    {
        const float SpeedMps = FMath::Max(Spell.SpeedMps, 0.0f);
        const float N = SpellParameterRanges::Normalize(
            SpeedMps,
            SpellParameterRanges::MinSpeedMps,
            SpellParameterRanges::MaxSpeedMps);

        // 0 speed -> exactly 0 rings.
        if (SpeedMps <= 0.1f || N <= 0.001f)
        {
            return 0;
        }
        if (N < (1.0f / 3.0f))
        {
            return 1;
        }
        if (N < (2.0f / 3.0f))
        {
            return 2;
        }
        return 3;
    }

    FVector RotateLocal(const FQuat& Rotation, const FVector& Local)
    {
        return Rotation.RotateVector(Local);
    }

    FVector ToWorld(const FVector& Center, const FQuat& Rotation, const FVector& Local)
    {
        return Center + RotateLocal(Rotation, Local);
    }

    void DrawPreviewCircle(
        UWorld* World,
        const FVector& Center,
        const float Radius,
        const FVector& PlaneAxisA,
        const FVector& PlaneAxisB,
        const FColor& Color,
        const float Thickness)
    {
        DrawDebugCircle(
            World,
            Center,
            FMath::Max(Radius, 1.0f),
            48,
            Color,
            false,
            0.0f,
            0,
            Thickness,
            PlaneAxisA,
            PlaneAxisB,
            false);
    }

    void DrawSphereDensityGrid(
        UWorld* World,
        const FVector& Center,
        const float Radius,
        const FQuat& Rotation,
        const int32 GridSteps,
        const FColor& Color)
    {
        const FVector X = RotateLocal(Rotation, FVector::ForwardVector);
        const FVector Y = RotateLocal(Rotation, FVector::RightVector);
        const FVector Z = RotateLocal(Rotation, FVector::UpVector);

        for (int32 I = 1; I < GridSteps; ++I)
        {
            const float T = -1.0f + 2.0f * (static_cast<float>(I) / static_cast<float>(GridSteps));
            const float LocalZ = Radius * T;
            const float CircleRadius = Radius * FMath::Sqrt(FMath::Max(0.0f, 1.0f - T * T));
            DrawPreviewCircle(World, Center + Z * LocalZ, CircleRadius, X, Y, Color, PreviewThickness);
        }

        for (int32 I = 0; I < GridSteps; ++I)
        {
            const float Angle = PI * static_cast<float>(I) / static_cast<float>(GridSteps);
            const FVector Horizontal = (X * FMath::Cos(Angle) + Y * FMath::Sin(Angle)).GetSafeNormal();
            DrawPreviewCircle(World, Center, Radius, Horizontal, Z, Color, PreviewThickness);
        }
    }

    void DrawCubeDensityGrid(
        UWorld* World,
        const FVector& Center,
        const FVector& Half,
        const FQuat& Rotation,
        const int32 GridSteps,
        const FColor& Color)
    {
        auto Line = [&](const FVector& A, const FVector& B)
        {
            DrawDebugLine(
                World,
                ToWorld(Center, Rotation, A),
                ToWorld(Center, Rotation, B),
                Color,
                false,
                0.0f,
                0,
                PreviewThickness);
        };

        for (int32 I = 1; I < GridSteps; ++I)
        {
            const float T = -1.0f + 2.0f * (static_cast<float>(I) / static_cast<float>(GridSteps));

            const float Y = Half.Y * T;
            const float Z = Half.Z * T;
            for (const float FaceX : {-Half.X, Half.X})
            {
                Line(FVector(FaceX, Y, -Half.Z), FVector(FaceX, Y, Half.Z));
                Line(FVector(FaceX, -Half.Y, Z), FVector(FaceX, Half.Y, Z));
            }

            const float X = Half.X * T;
            for (const float FaceY : {-Half.Y, Half.Y})
            {
                Line(FVector(X, FaceY, -Half.Z), FVector(X, FaceY, Half.Z));
                Line(FVector(-Half.X, FaceY, Z), FVector(Half.X, FaceY, Z));
            }

            for (const float FaceZ : {-Half.Z, Half.Z})
            {
                Line(FVector(X, -Half.Y, FaceZ), FVector(X, Half.Y, FaceZ));
                Line(FVector(-Half.X, Y, FaceZ), FVector(Half.X, Y, FaceZ));
            }
        }
    }

    void DrawConeDensityGrid(
        UWorld* World,
        const FVector& Center,
        const float Radius,
        const float Height,
        const FQuat& Rotation,
        const int32 GridSteps,
        const FColor& Color)
    {
        const FVector X = RotateLocal(Rotation, FVector::ForwardVector);
        const FVector Y = RotateLocal(Rotation, FVector::RightVector);
        const FVector Z = RotateLocal(Rotation, FVector::UpVector);
        const FVector Apex = Center + Z * (Height * 0.5f);
        const FVector BaseCenter = Center - Z * (Height * 0.5f);

        for (int32 I = 0; I < GridSteps; ++I)
        {
            const float Angle = 2.0f * PI * static_cast<float>(I) / static_cast<float>(GridSteps);
            const FVector Radial = X * FMath::Cos(Angle) + Y * FMath::Sin(Angle);
            DrawDebugLine(
                World,
                Apex,
                BaseCenter + Radial * Radius,
                Color,
                false,
                0.0f,
                0,
                PreviewThickness);
        }

        for (int32 I = 1; I <= GridSteps; ++I)
        {
            const float Alpha = static_cast<float>(I) / static_cast<float>(GridSteps);
            const FVector RingCenter = Apex - Z * (Height * Alpha);
            DrawPreviewCircle(World, RingCenter, Radius * Alpha, X, Y, Color, PreviewThickness);
        }
    }

    void DrawSpeedBands(
        UWorld* World,
        const FVector& Center,
        const FVector& HalfExtents,
        const FQuat& Rotation,
        const int32 RingCount,
        const FColor& Color)
    {
        if (RingCount <= 0)
        {
            return;
        }

        const FVector X = RotateLocal(Rotation, FVector::ForwardVector);
        const FVector Y = RotateLocal(Rotation, FVector::RightVector);
        const FVector Z = RotateLocal(Rotation, FVector::UpVector);

        const float RadiusAroundZ = FMath::Max(
            FMath::Sqrt(HalfExtents.X * HalfExtents.X + HalfExtents.Y * HalfExtents.Y) * 1.08f,
            12.0f);
        const float RadiusAroundY = FMath::Max(
            FMath::Sqrt(HalfExtents.X * HalfExtents.X + HalfExtents.Z * HalfExtents.Z) * 1.08f,
            12.0f);

        auto OffsetFraction = [RingCount](const int32 Index)
        {
            if (RingCount == 1)
            {
                return 0.0f;
            }
            if (RingCount == 2)
            {
                return Index == 0 ? -0.24f : 0.24f;
            }
            return Index == 0 ? -0.34f : (Index == 1 ? 0.0f : 0.34f);
        };

        for (int32 I = 0; I < RingCount; ++I)
        {
            const float Offset = OffsetFraction(I);

            DrawPreviewCircle(
                World,
                Center + Z * (HalfExtents.Z * Offset),
                RadiusAroundZ,
                X,
                Y,
                Color,
                SpeedRingThickness);

            DrawPreviewCircle(
                World,
                Center + Y * (HalfExtents.Y * Offset),
                RadiusAroundY,
                X,
                Z,
                Color,
                SpeedRingThickness);
        }
    }

    float GetShapeForwardClearanceCm(const FEarthSpellDefinition& Spell)
    {
        switch (Spell.Shape)
        {
            case EEarthSpellShape::Sphere:
                return FMath::Max(Spell.SphereRadiusCm, 1.0f);

            case EEarthSpellShape::Cube:
                return FMath::Max(
                    FMath::Max(Spell.CubeXcm, Spell.CubeYcm),
                    Spell.CubeZcm) * 0.5f;

            case EEarthSpellShape::Cone:
            default:
                return FMath::Max(
                    FMath::Max(Spell.ConeRadiusCm, Spell.ConeHeightCm * 0.5f),
                    1.0f);
        }
    }

    FQuat GetWorkbenchOrientationRotation(
        const FEarthSpellDefinition& Spell,
        const FQuat& BaseRotation)
    {
        if (Spell.Orientation == ESpellOrientationAxis::Up)
        {
            return BaseRotation;
        }

        FVector DesiredZ = Spell.Orientation == ESpellOrientationAxis::Forward
            ? BaseRotation.RotateVector(FVector::ForwardVector)
            : BaseRotation.RotateVector(FVector::RightVector);
        DesiredZ.Normalize();

        FVector ReferenceX = BaseRotation.RotateVector(FVector::UpVector);
        if (ReferenceX.IsNearlyZero() || FMath::Abs(FVector::DotProduct(DesiredZ, ReferenceX.GetSafeNormal())) > 0.98f)
        {
            ReferenceX = FVector::ForwardVector;
        }

        return FRotationMatrix::MakeFromZX(DesiredZ, ReferenceX).ToQuat();
    }

    void DrawSpellVisual(
        UWorld* World,
        const FEarthSpellDefinition& Spell,
        const FVector& Center,
        const FQuat& Rotation,
        const bool bRenderSpeedRings)
    {
        const int32 DensityGridSteps = GetDensityGridSteps(Spell);
        FVector PreviewHalfExtents(50.0f);

        switch (Spell.Shape)
        {
            case EEarthSpellShape::Sphere:
            {
                const float Radius = FMath::Max(Spell.SphereRadiusCm, 1.0f);
                PreviewHalfExtents = FVector(Radius);
                DrawSphereDensityGrid(World, Center, Radius, Rotation, DensityGridSteps, EarthGreen);
                break;
            }

            case EEarthSpellShape::Cube:
            {
                PreviewHalfExtents = FVector(
                    FMath::Max(Spell.CubeXcm, 1.0f) * 0.5f,
                    FMath::Max(Spell.CubeYcm, 1.0f) * 0.5f,
                    FMath::Max(Spell.CubeZcm, 1.0f) * 0.5f);

                DrawDebugBox(
                    World,
                    Center,
                    PreviewHalfExtents,
                    Rotation,
                    EarthGreen,
                    false,
                    0.0f,
                    0,
                    2.5f);

                DrawCubeDensityGrid(World, Center, PreviewHalfExtents, Rotation, DensityGridSteps, EarthGreen);
                break;
            }

            case EEarthSpellShape::Cone:
            default:
            {
                const float Height = FMath::Max(Spell.ConeHeightCm, 1.0f);
                const float Radius = FMath::Max(Spell.ConeRadiusCm, 1.0f);
                PreviewHalfExtents = FVector(Radius, Radius, Height * 0.5f);
                DrawConeDensityGrid(World, Center, Radius, Height, Rotation, DensityGridSteps, EarthGreen);
                break;
            }
        }

        const int32 SpeedRingCount = bRenderSpeedRings ? GetSpeedRingCount(Spell) : 0;
        DrawSpeedBands(World, Center, PreviewHalfExtents, Rotation, SpeedRingCount, SpeedGreen);
    }
}

TStatId USpellPreviewSubsystem::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(USpellPreviewSubsystem, STATGROUP_Tickables);
}

void USpellPreviewSubsystem::Tick(float DeltaSeconds)
{
    UWorld* World = GetWorld();
    if (!World || !World->IsGameWorld())
    {
        return;
    }

    /*
     * Workbench preview.
     *
     * SpellPreview OBSERVES InnerRealm instead of InnerRealm calling back into
     * SpellPreview. This preserves one-way module dependencies and avoids a
     * circular plugin graph.
     */
    const UInnerRealmSubsystem* InnerRealm = World->GetSubsystem<UInnerRealmSubsystem>();
    if (InnerRealm && InnerRealm->IsSpellWorkbenchVisible())
    {
        const USpellCreationSubsystem* SpellCreation = World->GetSubsystem<USpellCreationSubsystem>();
        AInnerRealmActor* RealmActor = nullptr;
        for (TActorIterator<AInnerRealmActor> It(World); It; ++It)
        {
            RealmActor = *It;
            break;
        }

        if (SpellCreation && RealmActor)
        {
            const FEarthSpellDefinition Spell = SpellCreation->GetStoredSpellDefinition();
            const FTransform WorkbenchReferenceTransform = RealmActor->GetPreviewReferenceTransform();

            FVector Forward = WorkbenchReferenceTransform.GetRotation().GetForwardVector();
            if (Forward.IsNearlyZero())
            {
                Forward = FVector::ForwardVector;
            }
            Forward.Normalize();

            const FVector ReferenceLocation = WorkbenchReferenceTransform.GetLocation();
            const float ShapeClearanceCm = GetShapeForwardClearanceCm(Spell);

            // Keep very large authored distances visible inside the Workbench.
            // The exact value remains visible in the editor text; only the visual
            // preview distance is compressed after 5 m.
            const float DisplayDistanceCm = FMath::Clamp(Spell.DistanceM * 100.0f, 0.0f, 500.0f);
            const FVector PreviewCenter =
                ReferenceLocation + Forward * (110.0f + ShapeClearanceCm + DisplayDistanceCm);

            DrawDebugLine(
                World,
                ReferenceLocation + FVector(0.0f, 0.0f, 25.0f),
                PreviewCenter,
                WorkbenchGuide,
                false,
                0.0f,
                0,
                1.0f);

            // The Workbench reflects the stored/default speed itself. This differs
            // intentionally from live casting, where rings appear only after the
            // live Speed modifier has been explicitly activated.
            TArray<FResolvedPatternInstance> PreviewInstances;
            FSpellPatternResolver::ResolveInstances(
                SpellCreation->GetStoredGenericSpellDefinition().Pattern,
                PreviewCenter,
                Forward,
                PreviewInstances);

            const FQuat BaseSpellRotation =
                GetWorkbenchOrientationRotation(
                    Spell,
                    WorkbenchReferenceTransform.GetRotation());

            for (const FResolvedPatternInstance& Instance : PreviewInstances)
            {
                const FQuat FinalRotation =
                    Instance.PatternRotation * BaseSpellRotation;

                DrawSpellVisual(
                    World,
                    Spell,
                    Instance.Location,
                    FinalRotation,
                    Spell.SpeedMps > 0.1f);
            }
        }
    }

    /*
     * Existing live-casting preview.
     */
    ULiveSpellSessionSubsystem* Session = World->GetSubsystem<ULiveSpellSessionSubsystem>();
    const UPlayerViewModeSubsystem* Views = World->GetSubsystem<UPlayerViewModeSubsystem>();
    APlayerController* PC = World->GetFirstPlayerController();
    APawn* Pawn = PC ? PC->GetPawn() : nullptr;

    if (!Session || !Views || !PC || !Pawn || !Views->IsTopDown() || !Session->HasConstruction())
    {
        return;
    }

    if (Session->IsEarthExplicitlySelected())
    {
        const FVector AuraCenter = Pawn->GetActorLocation() + FVector(0.0f, 0.0f, 75.0f);
        DrawDebugSphere(World, AuraCenter, 90.0f, 24, EarthGreen, false, 0.0f, 0, 2.0f);
        DrawDebugSphere(World, AuraCenter, 112.0f, 24, EarthGreen, false, 0.0f, 0, 1.0f);
    }

    if (!Session->HasExplicitShape())
    {
        return;
    }

    // The cleanup architecture made SpellExecution consume FResolvedSpell.
    // Keep the preview renderer on the legacy visual adapter for now, but pass
    // the canonical generic resolved spell into placement.
    const FResolvedSpell ResolvedSpell = Session->ResolveGenericSpell();
    const FEarthSpellDefinition Spell =
        FSpellDefinitionAdapter::ToLegacyEarth(ResolvedSpell.Definition);
    FVector AimOrigin;
    FVector AimDirection;
    if (!Views->GetTopDownCastRay(AimOrigin, AimDirection))
    {
        AimDirection = Pawn->GetActorForwardVector();
    }

    FResolvedSpellCastPlacement Placement;
    if (!FSpellCastPlacement::Resolve(PC, ResolvedSpell, AimDirection, Placement))
    {
        return;
    }

    // Live speed rings are an activation indicator, not a generic readout of
    // the persistent default speed.
    const bool bLiveSpeedActive = Session->IsParameterActive(ELiveSpellParameter::Speed);

    TArray<FResolvedPatternInstance> LivePreviewInstances;
    FSpellPatternResolver::ResolveInstances(
        ResolvedSpell.Definition.Pattern,
        Placement.SpawnLocation,
        AimDirection,
        LivePreviewInstances);

    const FQuat LiveBaseSpellRotation =
        Placement.SpawnRotation.Quaternion();

    for (const FResolvedPatternInstance& Instance : LivePreviewInstances)
    {
        const FQuat FinalRotation =
            Instance.PatternRotation * LiveBaseSpellRotation;

        DrawSpellVisual(
            World,
            Spell,
            Instance.Location,
            FinalRotation,
            bLiveSpeedActive);
    }
}
