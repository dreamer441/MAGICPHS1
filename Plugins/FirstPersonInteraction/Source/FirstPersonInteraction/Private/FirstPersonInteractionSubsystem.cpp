#include "FirstPersonInteractionSubsystem.h"

#include "Components/SkeletalMeshComponent.h"
#include "EarthBlockActor.h"
#include "EarthGeometryComponent.h"
#include "EarthMaterialComponent.h"
#include "EarthSpellBody.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "EngineUtils.h"
#include "MaterialPhysicalProperties.h"
#include "PlayerViewModeSubsystem.h"
#include "SpellDefinition.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"
#include "WorldCodexSubsystem.h"
#include "WorldCodexTypes.h"

namespace FirstPersonInteractionTuning
{
    // V016.7: much more responsive hand control.
    // V016.6 used 0.34 deg/pixel.
    constexpr float HandDegreesPerMousePixel = 1.20f;

    // Pressing a hand button raises that arm into a forward chest-ready pose.
    // Mouse movement then offsets this base direction along the shoulder arc.
    // Chest-level ready pose. The downward pitch keeps the hands below the
    // first-person eye line instead of lifting toward the face/sky.
    constexpr float ReadyPitchDeg = -22.0f;
    constexpr float LeftReadyYawDeg = -11.0f;
    constexpr float RightReadyYawDeg = 11.0f;

    constexpr float MinHorizontalOffsetDeg = -78.0f;
    constexpr float MaxHorizontalOffsetDeg = 78.0f;

    constexpr float MinVerticalOffsetDeg = -68.0f;
    constexpr float MaxVerticalOffsetDeg = 78.0f;

    // Fast but visible lift / return blend.
    constexpr float HandActivationInterpSpeed = 14.0f;

    // V016.7 was visually too tucked-in. 0.90 keeps a visible elbow bend but
    // places the hand much farther forward, roughly the intended "70% extended"
    // combat-ready look rather than a hand held against the torso.
    constexpr float ReachFraction = 0.90f;

    // Native look is suppressed while a hand is controlled, then we apply only
    // this small fraction of camera motion ourselves. This removes the frozen
    // view without making the camera chase the hand.
    constexpr float CameraDegreesPerMousePixel = 0.050f;
    constexpr float MinViewPitchDeg = -82.0f;
    constexpr float MaxViewPitchDeg = 82.0f;

    constexpr float ExamineDistanceCm = 600.0f;
    constexpr float AnalysisDisplaySeconds = 10.0f;
}

namespace
{
    FString Number(
        const float Value,
        const int32 DecimalPlaces = 2)
    {
        switch (DecimalPlaces)
        {
            case 0:
                return FString::Printf(TEXT("%.0f"), Value);

            case 1:
                return FString::Printf(TEXT("%.1f"), Value);

            case 2:
            default:
                return FString::Printf(TEXT("%.2f"), Value);
        }
    }

    FString CodexDisplayName(
        const UWorldCodexSubsystem* Codex,
        const FName ConceptId)
    {
        if (!Codex)
        {
            return ConceptId.ToString();
        }

        if (const FCodexEntry* Entry =
            Codex->FindEntry(ConceptId))
        {
            return Entry->DisplayName.IsEmpty()
                ? ConceptId.ToString()
                : Entry->DisplayName.ToString();
        }

        return ConceptId.ToString();
    }

    void AddCodexLine(
        FString& Text,
        const UWorldCodexSubsystem* Codex,
        const FName ConceptId,
        const FString& Value = FString())
    {
        if (!Codex || !Codex->FindEntry(ConceptId))
        {
            return;
        }

        Text += TEXT("\n");

        Text += CodexDisplayName(
            Codex,
            ConceptId);

        Text += FString::Printf(
            TEXT("  [%s]"),
            *ConceptId.ToString());

        if (!Value.IsEmpty())
        {
            Text += TEXT(": ");
            Text += Value;
        }
    }

    FName OrientationValueConcept(
        const ESpellOrientationAxis Axis)
    {
        switch (Axis)
        {
            case ESpellOrientationAxis::Forward:
                return FName(TEXT("orientation.forward"));
            case ESpellOrientationAxis::Right:
                return FName(TEXT("orientation.right"));
            case ESpellOrientationAxis::Up:
            default:
                return FName(TEXT("orientation.up"));
        }
    }

    FName MotionDirectionValueConcept(
        const ESpellMotionDirection Direction)
    {
        switch (Direction)
        {
            case ESpellMotionDirection::Backward:
                return FName(TEXT("direction.backward"));
            case ESpellMotionDirection::Up:
                return FName(TEXT("direction.up"));
            case ESpellMotionDirection::Down:
                return FName(TEXT("direction.down"));
            case ESpellMotionDirection::Outward:
                return FName(TEXT("direction.outward"));
            case ESpellMotionDirection::Inward:
                return FName(TEXT("direction.inward"));
            case ESpellMotionDirection::Tangent:
                return FName(TEXT("direction.tangent"));
            case ESpellMotionDirection::Forward:
            default:
                return FName(TEXT("direction.forward"));
        }
    }

    FVector BuildSphericalDirection(
        const FVector& Forward,
        const FVector& Right,
        const FVector& Up,
        const float YawDeg,
        const float PitchDeg)
    {
        const float YawRad =
            FMath::DegreesToRadians(YawDeg);

        const float PitchRad =
            FMath::DegreesToRadians(PitchDeg);

        const float CosPitch =
            FMath::Cos(PitchRad);

        return (
            Forward *
                (FMath::Cos(YawRad) * CosPitch) +
            Right *
                (FMath::Sin(YawRad) * CosPitch) +
            Up *
                FMath::Sin(PitchRad)
        ).GetSafeNormal();
    }
}

TStatId UFirstPersonInteractionSubsystem::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(
        UFirstPersonInteractionSubsystem,
        STATGROUP_Tickables);
}

void UFirstPersonInteractionSubsystem::Deinitialize()
{
    ExitFirstPerson();
    HideAnalysis();
    Super::Deinitialize();
}

void UFirstPersonInteractionSubsystem::AcquireLookLock(
    APlayerController* PC)
{
    if (!PC)
    {
        return;
    }

    if (bOwnsHandLookLock &&
        LookLockController.Get() != PC)
    {
        ReleaseLookLock();
    }

    if (!bOwnsHandLookLock)
    {
        // Only native look is paused. WASD / jump continue to run normally.
        PC->SetIgnoreLookInput(true);
        LookLockController = PC;
        bOwnsHandLookLock = true;
    }
}

void UFirstPersonInteractionSubsystem::ReleaseLookLock()
{
    if (!bOwnsHandLookLock)
    {
        return;
    }

    if (APlayerController* PC =
        LookLockController.Get())
    {
        PC->SetIgnoreLookInput(false);
    }

    bOwnsHandLookLock = false;
    LookLockController.Reset();
}

void UFirstPersonInteractionSubsystem::BindCharacterMesh(
    APlayerController* PC)
{
    ACharacter* Character =
        PC ? Cast<ACharacter>(PC->GetPawn()) : nullptr;

    USkeletalMeshComponent* Mesh =
        Character ? Character->GetMesh() : nullptr;

    if (!Mesh)
    {
        UnbindCharacterMesh();
        return;
    }

    if (BoundMesh.Get() == Mesh &&
        BoneTransformsFinalizedHandle.IsValid())
    {
        BoundController = PC;
        return;
    }

    UnbindCharacterMesh();

    BoundMesh = Mesh;
    BoundController = PC;

    bArmBonesValid =
        ResolveArmBones();

    if (!bArmBonesValid)
    {
        if (!bBoneStatusReported && GEngine)
        {
            GEngine->AddOnScreenDebugMessage(
                -1,
                5.0f,
                FColor::Red,
                TEXT("STATE 1 ARM IK: Quinn arm bones were not found."));
        }

        bBoneStatusReported = true;
        return;
    }

    BoneTransformsFinalizedHandle =
        Mesh->RegisterOnBoneTransformsFinalizedDelegate(
            FOnBoneTransformsFinalizedMultiCast::FDelegate::CreateUObject(
                this,
                &UFirstPersonInteractionSubsystem::
                    OnBoneTransformsFinalized));

    if (!bBoneStatusReported && GEngine)
    {
        GEngine->AddOnScreenDebugMessage(
            -1,
            3.0f,
            FColor::Green,
            TEXT("STATE 1: Quinn arm IK active"));
    }

    bBoneStatusReported = true;
}

void UFirstPersonInteractionSubsystem::UnbindCharacterMesh()
{
    if (USkeletalMeshComponent* Mesh =
        BoundMesh.Get())
    {
        if (BoneTransformsFinalizedHandle.IsValid())
        {
            Mesh->UnregisterOnBoneTransformsFinalizedDelegate(
                BoneTransformsFinalizedHandle);
        }
    }

    BoneTransformsFinalizedHandle.Reset();
    BoundMesh.Reset();
    BoundController.Reset();

    LeftUpperArmIndex = INDEX_NONE;
    LeftLowerArmIndex = INDEX_NONE;
    LeftHandIndex = INDEX_NONE;

    RightUpperArmIndex = INDEX_NONE;
    RightLowerArmIndex = INDEX_NONE;
    RightHandIndex = INDEX_NONE;

    bArmBonesValid = false;
    bApplyingArmPose = false;
}

bool UFirstPersonInteractionSubsystem::ResolveArmBones()
{
    USkeletalMeshComponent* Mesh =
        BoundMesh.Get();

    if (!Mesh)
    {
        return false;
    }

    // Standard UE5 Manny/Quinn mannequin names.
    LeftUpperArmIndex =
        Mesh->GetBoneIndex(
            FName(TEXT("upperarm_l")));

    LeftLowerArmIndex =
        Mesh->GetBoneIndex(
            FName(TEXT("lowerarm_l")));

    LeftHandIndex =
        Mesh->GetBoneIndex(
            FName(TEXT("hand_l")));

    RightUpperArmIndex =
        Mesh->GetBoneIndex(
            FName(TEXT("upperarm_r")));

    RightLowerArmIndex =
        Mesh->GetBoneIndex(
            FName(TEXT("lowerarm_r")));

    RightHandIndex =
        Mesh->GetBoneIndex(
            FName(TEXT("hand_r")));

    return
        LeftUpperArmIndex != INDEX_NONE &&
        LeftLowerArmIndex != INDEX_NONE &&
        LeftHandIndex != INDEX_NONE &&
        RightUpperArmIndex != INDEX_NONE &&
        RightLowerArmIndex != INDEX_NONE &&
        RightHandIndex != INDEX_NONE;
}

void UFirstPersonInteractionSubsystem::EnterFirstPerson(
    APlayerController* PC)
{
    LeftHandAnglesDeg =
        FVector2D::ZeroVector;

    RightHandAnglesDeg =
        FVector2D::ZeroVector;

    LeftHandVelocityCmS =
        FVector::ZeroVector;

    RightHandVelocityCmS =
        FVector::ZeroVector;

    LeftHandIKAlpha = 0.0f;
    RightHandIKAlpha = 0.0f;

    bHavePreviousHandLocations = false;
    bBoneStatusReported = false;

    // Kill any old V016.5 proxy-hand actors that could have survived an editor
    // hot reload. Normal project restart would already remove them.
    if (UWorld* World = GetWorld())
    {
        TArray<AActor*> ToDestroy;

        for (TActorIterator<AActor> It(World); It; ++It)
        {
            if (It->GetClass()->GetName().Contains(
                TEXT("FirstPersonHandProxy")))
            {
                ToDestroy.Add(*It);
            }
        }

        for (AActor* Actor : ToDestroy)
        {
            if (Actor)
            {
                Actor->Destroy();
            }
        }
    }

    BindCharacterMesh(PC);
    bWasFirstPerson = true;
}

void UFirstPersonInteractionSubsystem::ExitFirstPerson()
{
    ReleaseLookLock();
    UnbindCharacterMesh();

    LeftHandAnglesDeg =
        FVector2D::ZeroVector;

    RightHandAnglesDeg =
        FVector2D::ZeroVector;

    LeftHandIKAlpha = 0.0f;
    RightHandIKAlpha = 0.0f;

    HideAnalysis();
    bWasFirstPerson = false;
}

void UFirstPersonInteractionSubsystem::UpdateHandInput(
    APlayerController* PC,
    const float DeltaSeconds)
{
    if (!PC)
    {
        return;
    }

    const bool bLeftActive =
        PC->IsInputKeyDown(
            EKeys::LeftMouseButton);

    const bool bRightActive =
        PC->IsInputKeyDown(
            EKeys::RightMouseButton);

    const bool bLeftJustPressed =
        PC->WasInputKeyJustPressed(
            EKeys::LeftMouseButton);

    const bool bRightJustPressed =
        PC->WasInputKeyJustPressed(
            EKeys::RightMouseButton);

    const bool bLeftJustReleased =
        PC->WasInputKeyJustReleased(
            EKeys::LeftMouseButton);

    const bool bRightJustReleased =
        PC->WasInputKeyJustReleased(
            EKeys::RightMouseButton);

    const bool bAnyHandActive =
        bLeftActive ||
        bRightActive;

    const bool bAnyHandJustPressed =
        bLeftJustPressed ||
        bRightJustPressed;

    // Every activation starts from a clean chest-ready pose. No previous
    // procedural angle is allowed to leak into the next hand use.
    if (bLeftJustPressed)
    {
        LeftHandAnglesDeg =
            FVector2D::ZeroVector;
    }

    if (bRightJustPressed)
    {
        RightHandAnglesDeg =
            FVector2D::ZeroVector;
    }

    // Release fully discards the procedural target. IK influence blends out
    // into Quinn's untouched ABP_Unarmed pose.
    if (bLeftJustReleased)
    {
        LeftHandAnglesDeg =
            FVector2D::ZeroVector;
    }

    if (bRightJustReleased)
    {
        RightHandAnglesDeg =
            FVector2D::ZeroVector;
    }

    LeftHandIKAlpha =
        FMath::FInterpTo(
            LeftHandIKAlpha,
            bLeftActive ? 1.0f : 0.0f,
            DeltaSeconds,
            FirstPersonInteractionTuning::
                HandActivationInterpSpeed);

    RightHandIKAlpha =
        FMath::FInterpTo(
            RightHandIKAlpha,
            bRightActive ? 1.0f : 0.0f,
            DeltaSeconds,
            FirstPersonInteractionTuning::
                HandActivationInterpSpeed);

    if (!bLeftActive &&
        LeftHandIKAlpha < 0.002f)
    {
        LeftHandIKAlpha = 0.0f;
    }

    if (!bRightActive &&
        RightHandIKAlpha < 0.002f)
    {
        RightHandIKAlpha = 0.0f;
    }

    // Suppress the full-speed native camera while a hand is active, but DO NOT
    // freeze the view. We manually apply a very small camera response below.
    if (bAnyHandActive)
    {
        AcquireLookLock(PC);
    }
    else
    {
        ReleaseLookLock();
    }

    float MouseX = 0.0f;
    float MouseY = 0.0f;

    PC->GetInputMouseDelta(
        MouseX,
        MouseY);

    // Mouse-button capture can report a large one-frame delta exactly when a
    // button is pressed. That was the remaining source of the "snap to sky".
    // Activation itself should only raise the arm; movement begins next frame.
    if (bAnyHandJustPressed)
    {
        MouseX = 0.0f;
        MouseY = 0.0f;
    }

    auto MoveHand =
        [MouseX, MouseY](
            FVector2D& Angles,
            const bool bActive)
        {
            if (!bActive)
            {
                return;
            }

            Angles.X =
                FMath::Clamp(
                    Angles.X +
                        MouseX *
                        FirstPersonInteractionTuning::
                            HandDegreesPerMousePixel,
                    FirstPersonInteractionTuning::
                        MinHorizontalOffsetDeg,
                    FirstPersonInteractionTuning::
                        MaxHorizontalOffsetDeg);

            // Intentionally parallel/non-inverted vertical mapping.
            Angles.Y =
                FMath::Clamp(
                    Angles.Y +
                        MouseY *
                        FirstPersonInteractionTuning::
                            HandDegreesPerMousePixel,
                    FirstPersonInteractionTuning::
                        MinVerticalOffsetDeg,
                    FirstPersonInteractionTuning::
                        MaxVerticalOffsetDeg);
        };

    MoveHand(
        LeftHandAnglesDeg,
        bLeftActive);

    MoveHand(
        RightHandAnglesDeg,
        bRightActive);

    if (bAnyHandActive &&
        (!FMath::IsNearlyZero(MouseX) ||
         !FMath::IsNearlyZero(MouseY)))
    {
        // Soft camera follow only. There is NO edge overflow / snap transfer.
        FRotator ControlRotation =
            PC->GetControlRotation();

        ControlRotation.Yaw +=
            MouseX *
            FirstPersonInteractionTuning::
                CameraDegreesPerMousePixel;

        // Unreal can represent a small downward pitch as ~350 degrees.
        // Clamping that wrapped value directly to [-82, 82] snaps it to +82
        // (the sky). Normalize to signed degrees before applying/clamping.
        const float SignedPitch =
            FMath::UnwindDegrees(
                ControlRotation.Pitch);

        ControlRotation.Pitch =
            FMath::Clamp(
                SignedPitch +
                    MouseY *
                    FirstPersonInteractionTuning::
                        CameraDegreesPerMousePixel,
                FirstPersonInteractionTuning::
                    MinViewPitchDeg,
                FirstPersonInteractionTuning::
                    MaxViewPitchDeg);

        PC->SetControlRotation(
            ControlRotation);
    }
}

bool UFirstPersonInteractionSubsystem::IsBoneDescendantOf(
    const int32 BoneIndex,
    const int32 AncestorBoneIndex) const
{
    const USkeletalMeshComponent* Mesh =
        BoundMesh.Get();

    if (!Mesh ||
        BoneIndex == INDEX_NONE ||
        AncestorBoneIndex == INDEX_NONE)
    {
        return false;
    }

    const FName AncestorName =
        Mesh->GetBoneName(
            AncestorBoneIndex);

    FName CurrentName =
        Mesh->GetBoneName(
            BoneIndex);

    while (CurrentName != NAME_None)
    {
        if (CurrentName == AncestorName)
        {
            return true;
        }

        CurrentName =
            Mesh->GetParentBone(
                CurrentName);
    }

    return false;
}

void UFirstPersonInteractionSubsystem::RotateBoneSubtree(
    const int32 AncestorBoneIndex,
    const FVector& PivotComponentSpace,
    const FQuat& DeltaRotation,
    TArray<FTransform>& ComponentSpaceTransforms) const
{
    if (DeltaRotation.IsIdentity())
    {
        return;
    }

    const int32 Count =
        ComponentSpaceTransforms.Num();

    for (int32 BoneIndex = 0;
         BoneIndex < Count;
         ++BoneIndex)
    {
        if (!IsBoneDescendantOf(
            BoneIndex,
            AncestorBoneIndex))
        {
            continue;
        }

        FTransform& BoneTransform =
            ComponentSpaceTransforms[
                BoneIndex];

        const FVector Relative =
            BoneTransform.GetLocation() -
            PivotComponentSpace;

        BoneTransform.SetLocation(
            PivotComponentSpace +
            DeltaRotation.RotateVector(
                Relative));

        FQuat NewRotation =
            DeltaRotation *
            BoneTransform.GetRotation();

        NewRotation.Normalize();

        BoneTransform.SetRotation(
            NewRotation);
    }
}

void UFirstPersonInteractionSubsystem::ApplyArmIK(
    const bool bLeftArm,
    TArray<FTransform>& ComponentSpaceTransforms)
{
    USkeletalMeshComponent* Mesh =
        BoundMesh.Get();

    APlayerController* PC =
        BoundController.Get();

    if (!Mesh ||
        !PC ||
        !bArmBonesValid)
    {
        return;
    }

    const float IKAlpha =
        bLeftArm
            ? LeftHandIKAlpha
            : RightHandIKAlpha;

    // At alpha zero we leave ABP_Unarmed completely untouched.
    if (IKAlpha <= 0.001f)
    {
        return;
    }

    const int32 UpperIndex =
        bLeftArm
            ? LeftUpperArmIndex
            : RightUpperArmIndex;

    const int32 LowerIndex =
        bLeftArm
            ? LeftLowerArmIndex
            : RightLowerArmIndex;

    const int32 HandIndex =
        bLeftArm
            ? LeftHandIndex
            : RightHandIndex;

    if (!ComponentSpaceTransforms.IsValidIndex(
            UpperIndex) ||
        !ComponentSpaceTransforms.IsValidIndex(
            LowerIndex) ||
        !ComponentSpaceTransforms.IsValidIndex(
            HandIndex))
    {
        return;
    }

    const FVector Shoulder =
        ComponentSpaceTransforms[
            UpperIndex].GetLocation();

    const FVector Elbow =
        ComponentSpaceTransforms[
            LowerIndex].GetLocation();

    const FVector Hand =
        ComponentSpaceTransforms[
            HandIndex].GetLocation();

    const float UpperLength =
        FVector::Distance(
            Shoulder,
            Elbow);

    const float LowerLength =
        FVector::Distance(
            Elbow,
            Hand);

    if (UpperLength < 1.0f ||
        LowerLength < 1.0f)
    {
        return;
    }

    FVector ViewLocation;
    FRotator ViewRotation;

    PC->GetPlayerViewPoint(
        ViewLocation,
        ViewRotation);

    const FVector ForwardWorld =
        ViewRotation.Vector();

    const FVector RightWorld =
        FRotationMatrix(
            ViewRotation)
            .GetUnitAxis(
                EAxis::Y);

    const FVector UpWorld =
        FRotationMatrix(
            ViewRotation)
            .GetUnitAxis(
                EAxis::Z);

    const FTransform MeshWorld =
        Mesh->GetComponentTransform();

    const FVector Forward =
        MeshWorld.InverseTransformVectorNoScale(
            ForwardWorld)
            .GetSafeNormal();

    const FVector Right =
        MeshWorld.InverseTransformVectorNoScale(
            RightWorld)
            .GetSafeNormal();

    const FVector Up =
        MeshWorld.InverseTransformVectorNoScale(
            UpWorld)
            .GetSafeNormal();

    const FVector2D& Angles =
        bLeftArm
            ? LeftHandAnglesDeg
            : RightHandAnglesDeg;

    const float ReadyYaw =
        bLeftArm
            ? FirstPersonInteractionTuning::
                LeftReadyYawDeg
            : FirstPersonInteractionTuning::
                RightReadyYawDeg;

    const float YawDeg =
        ReadyYaw +
        Angles.X;

    const float PitchDeg =
        FirstPersonInteractionTuning::
            ReadyPitchDeg +
        Angles.Y;

    const FVector TargetDirection =
        BuildSphericalDirection(
            Forward,
            Right,
            Up,
            YawDeg,
            PitchDeg);

    const float TotalReach =
        UpperLength +
        LowerLength;

    const float DesiredDistance =
        TotalReach *
        FirstPersonInteractionTuning::
            ReachFraction;

    const float MinDistance =
        FMath::Abs(
            UpperLength -
            LowerLength) +
        0.5f;

    const float MaxDistance =
        FMath::Max(
            MinDistance,
            TotalReach - 0.5f);

    const float ReachDistance =
        FMath::Clamp(
            DesiredDistance,
            MinDistance,
            MaxDistance);

    const FVector ProceduralTarget =
        Shoulder +
        TargetDirection *
        ReachDistance;

    // Blend from the current animation hand location into the chest/gesture
    // target. On release IKAlpha falls to zero, which naturally restores the
    // exact original animated pose instead of leaving the arm twisted.
    const FVector Target =
        FMath::Lerp(
            Hand,
            ProceduralTarget,
            IKAlpha);

    const FVector ShoulderToTarget =
        Target -
        Shoulder;

    const float Distance =
        ShoulderToTarget.Size();

    if (Distance < KINDA_SMALL_NUMBER)
    {
        return;
    }

    const FVector AimDirection =
        ShoulderToTarget /
        Distance;

    // Stable human-like elbow pole: down + outward from the torso.
    const float SideSign =
        bLeftArm ? -1.0f : 1.0f;

    FVector PoleDirection =
        -Up * 0.78f +
        Right *
            SideSign *
            0.48f;

    // Project pole onto the plane perpendicular to the reach direction.
    PoleDirection -=
        AimDirection *
        FVector::DotProduct(
            PoleDirection,
            AimDirection);

    if (!PoleDirection.Normalize())
    {
        PoleDirection =
            Right * SideSign;

        PoleDirection -=
            AimDirection *
            FVector::DotProduct(
                PoleDirection,
                AimDirection);

        PoleDirection.Normalize();
    }

    const float CosShoulder =
        FMath::Clamp(
            (
                UpperLength * UpperLength +
                Distance * Distance -
                LowerLength * LowerLength
            ) /
            (
                2.0f *
                UpperLength *
                Distance
            ),
            -1.0f,
            1.0f);

    const float Along =
        UpperLength *
        CosShoulder;

    const float HeightSquared =
        FMath::Max(
            0.0f,
            UpperLength * UpperLength -
            Along * Along);

    const float Height =
        FMath::Sqrt(
            HeightSquared);

    const FVector DesiredElbow =
        Shoulder +
        AimDirection *
            Along +
        PoleDirection *
            Height;

    const FVector CurrentUpperDirection =
        (Elbow - Shoulder)
            .GetSafeNormal();

    const FVector DesiredUpperDirection =
        (DesiredElbow - Shoulder)
            .GetSafeNormal();

    if (!CurrentUpperDirection.IsNearlyZero() &&
        !DesiredUpperDirection.IsNearlyZero())
    {
        const FQuat UpperDelta =
            FQuat::FindBetweenNormals(
                CurrentUpperDirection,
                DesiredUpperDirection);

        RotateBoneSubtree(
            UpperIndex,
            Shoulder,
            UpperDelta,
            ComponentSpaceTransforms);
    }

    // The first rotation moved the whole forearm subtree. Read its new pose,
    // then rotate the forearm around the solved elbow to land the hand target.
    const FVector SolvedElbow =
        ComponentSpaceTransforms[
            LowerIndex].GetLocation();

    const FVector RotatedHand =
        ComponentSpaceTransforms[
            HandIndex].GetLocation();

    const FVector CurrentLowerDirection =
        (RotatedHand - SolvedElbow)
            .GetSafeNormal();

    const FVector DesiredLowerDirection =
        (Target - SolvedElbow)
            .GetSafeNormal();

    if (!CurrentLowerDirection.IsNearlyZero() &&
        !DesiredLowerDirection.IsNearlyZero())
    {
        const FQuat LowerDelta =
            FQuat::FindBetweenNormals(
                CurrentLowerDirection,
                DesiredLowerDirection);

        RotateBoneSubtree(
            LowerIndex,
            SolvedElbow,
            LowerDelta,
            ComponentSpaceTransforms);
    }
}

void UFirstPersonInteractionSubsystem::UpdateHandTrajectoryState(
    const TArray<FTransform>& ComponentSpaceTransforms)
{
    USkeletalMeshComponent* Mesh =
        BoundMesh.Get();

    UWorld* World =
        GetWorld();

    if (!Mesh ||
        !World ||
        !ComponentSpaceTransforms.IsValidIndex(
            LeftHandIndex) ||
        !ComponentSpaceTransforms.IsValidIndex(
            RightHandIndex))
    {
        return;
    }

    const FTransform MeshWorld =
        Mesh->GetComponentTransform();

    const FVector NewLeft =
        MeshWorld.TransformPosition(
            ComponentSpaceTransforms[
                LeftHandIndex].GetLocation());

    const FVector NewRight =
        MeshWorld.TransformPosition(
            ComponentSpaceTransforms[
                RightHandIndex].GetLocation());

    const float DeltaSeconds =
        FMath::Max(
            World->GetDeltaSeconds(),
            KINDA_SMALL_NUMBER);

    if (bHavePreviousHandLocations)
    {
        LeftHandVelocityCmS =
            (NewLeft -
             LeftHandWorldLocation) /
            DeltaSeconds;

        RightHandVelocityCmS =
            (NewRight -
             RightHandWorldLocation) /
            DeltaSeconds;
    }
    else
    {
        LeftHandVelocityCmS =
            FVector::ZeroVector;

        RightHandVelocityCmS =
            FVector::ZeroVector;

        bHavePreviousHandLocations =
            true;
    }

    LeftHandWorldLocation =
        NewLeft;

    RightHandWorldLocation =
        NewRight;
}

void UFirstPersonInteractionSubsystem::OnBoneTransformsFinalized()
{
    if (!bWasFirstPerson ||
        bApplyingArmPose ||
        !bArmBonesValid)
    {
        return;
    }

    USkeletalMeshComponent* Mesh =
        BoundMesh.Get();

    if (!Mesh)
    {
        return;
    }

    TArray<FTransform>& ComponentSpaceTransforms =
        Mesh->GetEditableComponentSpaceTransforms();

    if (ComponentSpaceTransforms.Num() == 0)
    {
        return;
    }

    bApplyingArmPose = true;

    const bool bAnyArmIK =
        LeftHandIKAlpha > 0.001f ||
        RightHandIKAlpha > 0.001f;

    if (bAnyArmIK)
    {
        ApplyArmIK(
            true,
            ComponentSpaceTransforms);

        ApplyArmIK(
            false,
            ComponentSpaceTransforms);
    }

    UpdateHandTrajectoryState(
        ComponentSpaceTransforms);

    if (bAnyArmIK)
    {
        // UE exposes this specifically for external component-space pose edits.
        Mesh->ApplyEditedComponentSpaceTransforms();
    }

    bApplyingArmPose = false;
}

bool UFirstPersonInteractionSubsystem::BuildAnalysisText(
    AActor* Target,
    FString& OutText) const
{
    if (!Target)
    {
        return false;
    }

    UWorld* World = GetWorld();

    const UWorldCodexSubsystem* Codex =
        World
            ? World->GetSubsystem<
                UWorldCodexSubsystem>()
            : nullptr;

    if (!Codex)
    {
        return false;
    }

    if (const AEarthSpellBody* Body =
        Cast<AEarthSpellBody>(Target))
    {
        const FResolvedSpell& Spell =
            Body->GetRuntimeSpell();

        const FSpellDefinition& D =
            Spell.Definition;

        OutText =
            TEXT("ANALYSIS — EARTH CONSTRUCT");

        AddCodexLine(
            OutText,
            Codex,
            FName(TEXT("element.earth")));

        switch (D.Shape)
        {
            case ESpellShape::Sphere:
                AddCodexLine(
                    OutText,
                    Codex,
                    FName(TEXT("shape.sphere")));

                AddCodexLine(
                    OutText,
                    Codex,
                    FName(TEXT("shape.sphere.radius")),
                    Number(
                        D.ShapeDefinition.SphereRadiusCm,
                        1) +
                        TEXT(" cm"));
                break;

            case ESpellShape::Cube:
                AddCodexLine(
                    OutText,
                    Codex,
                    FName(TEXT("shape.cube")));

                AddCodexLine(
                    OutText,
                    Codex,
                    FName(TEXT("shape.cube.x")),
                    Number(
                        D.ShapeDefinition.CubeXcm,
                        1) +
                        TEXT(" cm"));

                AddCodexLine(
                    OutText,
                    Codex,
                    FName(TEXT("shape.cube.y")),
                    Number(
                        D.ShapeDefinition.CubeYcm,
                        1) +
                        TEXT(" cm"));

                AddCodexLine(
                    OutText,
                    Codex,
                    FName(TEXT("shape.cube.z")),
                    Number(
                        D.ShapeDefinition.CubeZcm,
                        1) +
                        TEXT(" cm"));
                break;

            case ESpellShape::Cone:
            default:
                AddCodexLine(
                    OutText,
                    Codex,
                    FName(TEXT("shape.cone")));

                AddCodexLine(
                    OutText,
                    Codex,
                    FName(TEXT("shape.cone.radius")),
                    Number(
                        D.ShapeDefinition.ConeRadiusCm,
                        1) +
                        TEXT(" cm"));

                AddCodexLine(
                    OutText,
                    Codex,
                    FName(TEXT("shape.cone.height")),
                    Number(
                        D.ShapeDefinition.ConeHeightCm,
                        1) +
                        TEXT(" cm"));
                break;
        }

        AddCodexLine(
            OutText,
            Codex,
            FName(TEXT("physics.mass")),
            Number(
                Body->GetBodyMassKg(),
                1) +
                TEXT(" kg"));

        AddCodexLine(
            OutText,
            Codex,
            FName(TEXT("material.density")),
            Number(
                D.Material.DensityKgPerM3,
                1) +
                TEXT(" kg/m^3"));

        AddCodexLine(
            OutText,
            Codex,
            FName(TEXT("material.hardness")),
            Number(
                D.Material.Hardness));

        AddCodexLine(
            OutText,
            Codex,
            FName(TEXT("material.toughness")),
            Number(
                D.Material.Toughness));

        AddCodexLine(
            OutText,
            Codex,
            FName(TEXT("material.elasticity")),
            Number(
                D.Material.Restitution));

        AddCodexLine(
            OutText,
            Codex,
            FName(TEXT("spatial.distance")),
            Number(
                D.DistanceM,
                1) +
                TEXT(" m"));

        AddCodexLine(
            OutText,
            Codex,
            FName(TEXT("spatial.orientation")),
            CodexDisplayName(
                Codex,
                OrientationValueConcept(
                    D.Orientation)));

        AddCodexLine(
            OutText,
            Codex,
            FName(TEXT("motion.speed")),
            Number(
                D.SpeedMps,
                1) +
                TEXT(" m/s"));

        AddCodexLine(
            OutText,
            Codex,
            FName(TEXT("motion.direction")),
            CodexDisplayName(
                Codex,
                MotionDirectionValueConcept(
                    D.MotionDirection)));

        AddCodexLine(
            OutText,
            Codex,
            FName(TEXT("pattern.amount")),
            FString::FromInt(
                D.Pattern.Amount));

        if (D.Pattern.Amount > 1)
        {
            if (D.Pattern.Arrangement ==
                ESpellArrangement::Circle)
            {
                AddCodexLine(
                    OutText,
                    Codex,
                    FName(TEXT("pattern.circle")));

                AddCodexLine(
                    OutText,
                    Codex,
                    FName(TEXT("pattern.circle_radius")),
                    Number(
                        D.Pattern.CircleRadiusCm,
                        1) +
                        TEXT(" cm"));
            }
            else
            {
                AddCodexLine(
                    OutText,
                    Codex,
                    FName(TEXT("pattern.line")));

                AddCodexLine(
                    OutText,
                    Codex,
                    FName(TEXT("pattern.spacing")),
                    Number(
                        D.Pattern.SpacingCm,
                        1) +
                        TEXT(" cm"));
            }
        }

        return true;
    }

    if (const AEarthBlockActor* Block =
        Cast<AEarthBlockActor>(Target))
    {
        if (!Block->MaterialState ||
            !Block->Geometry)
        {
            return false;
        }

        const FMaterialPhysicalProperties Material =
            Block->MaterialState->
                GetMaterialState();

        const FVector SizeCm =
            Block->Geometry->
                GetResolvedBlockSizeCm();

        const float VolumeM3 =
            FMath::Max(
                SizeCm.X,
                0.0f) *
            FMath::Max(
                SizeCm.Y,
                0.0f) *
            FMath::Max(
                SizeCm.Z,
                0.0f) /
            1000000.0f;

        const float MassKg =
            VolumeM3 *
            Material.DensityKgPerM3;

        OutText =
            TEXT("ANALYSIS — EARTH BLOCK");

        AddCodexLine(
            OutText,
            Codex,
            FName(TEXT("element.earth")));

        AddCodexLine(
            OutText,
            Codex,
            FName(TEXT("shape.cube")));

        AddCodexLine(
            OutText,
            Codex,
            FName(TEXT("shape.cube.x")),
            Number(SizeCm.X, 1) +
                TEXT(" cm"));

        AddCodexLine(
            OutText,
            Codex,
            FName(TEXT("shape.cube.y")),
            Number(SizeCm.Y, 1) +
                TEXT(" cm"));

        AddCodexLine(
            OutText,
            Codex,
            FName(TEXT("shape.cube.z")),
            Number(SizeCm.Z, 1) +
                TEXT(" cm"));

        AddCodexLine(
            OutText,
            Codex,
            FName(TEXT("physics.mass")),
            Number(MassKg, 1) +
                TEXT(" kg"));

        AddCodexLine(
            OutText,
            Codex,
            FName(TEXT("material.density")),
            Number(
                Material.DensityKgPerM3,
                1) +
                TEXT(" kg/m^3"));

        AddCodexLine(
            OutText,
            Codex,
            FName(TEXT("material.hardness")),
            Number(
                Material.Hardness));

        AddCodexLine(
            OutText,
            Codex,
            FName(TEXT("material.toughness")),
            Number(
                Material.Toughness));

        AddCodexLine(
            OutText,
            Codex,
            FName(TEXT("material.elasticity")),
            Number(
                Material.Restitution));

        return true;
    }

    return false;
}

void UFirstPersonInteractionSubsystem::TryExamine(
    APlayerController* PC)
{
    if (!PC || !PC->GetWorld())
    {
        return;
    }

    FVector ViewLocation;
    FRotator ViewRotation;
    PC->GetPlayerViewPoint(
        ViewLocation,
        ViewRotation);

    const FVector End =
        ViewLocation +
        ViewRotation.Vector() *
            FirstPersonInteractionTuning::
                ExamineDistanceCm;

    FCollisionQueryParams Params(
        SCENE_QUERY_STAT(
            FirstPersonExamine),
        true,
        PC->GetPawn());

    FHitResult Hit;

    if (!PC->GetWorld()->
        LineTraceSingleByChannel(
            Hit,
            ViewLocation,
            End,
            ECC_Visibility,
            Params))
    {
        ShowAnalysis(
            TEXT("ANALYSIS\nNo readable object in range."));
        return;
    }

    FString Text;

    if (!BuildAnalysisText(
        Hit.GetActor(),
        Text))
    {
        ShowAnalysis(
            FString::Printf(
                TEXT("ANALYSIS — %s\nNo current Codex-readable properties."),
                Hit.GetActor()
                    ? *Hit.GetActor()->GetName()
                    : TEXT("Unknown")));
        return;
    }

    ShowAnalysis(Text);
}

void UFirstPersonInteractionSubsystem::ShowAnalysis(
    const FString& Text)
{
    HideAnalysis();

    if (!GEngine ||
        !GEngine->GameViewport)
    {
        return;
    }

    AnalysisWidget =
        SNew(SOverlay)

        + SOverlay::Slot()
        .HAlign(HAlign_Left)
        .VAlign(VAlign_Top)
        .Padding(
            FMargin(
                28.0f,
                70.0f,
                0.0f,
                0.0f))
        [
            SNew(SBox)
            .WidthOverride(500.0f)
            [
                SNew(SBorder)
                .Padding(14.0f)
                .BorderBackgroundColor(
                    FLinearColor(
                        0.02f,
                        0.02f,
                        0.02f,
                        0.78f))
                [
                    SNew(STextBlock)
                    .Text(
                        FText::FromString(
                            Text))
                    .AutoWrapText(true)
                    .Font(
                        FCoreStyle::
                            GetDefaultFontStyle(
                                "Regular",
                                13))
                ]
            ]
        ];

    GEngine->GameViewport->
        AddViewportWidgetContent(
            AnalysisWidget.ToSharedRef(),
            900);

    AnalysisRemainingSeconds =
        FirstPersonInteractionTuning::
            AnalysisDisplaySeconds;
}

void UFirstPersonInteractionSubsystem::HideAnalysis()
{
    if (AnalysisWidget.IsValid())
    {
        if (GEngine &&
            GEngine->GameViewport)
        {
            GEngine->GameViewport->
                RemoveViewportWidgetContent(
                    AnalysisWidget.ToSharedRef());
        }

        AnalysisWidget.Reset();
    }

    AnalysisRemainingSeconds = 0.0f;
}

void UFirstPersonInteractionSubsystem::Tick(
    const float DeltaSeconds)
{
    UWorld* World = GetWorld();

    if (!World ||
        !World->IsGameWorld())
    {
        return;
    }

    APlayerController* PC =
        World->GetFirstPlayerController();

    if (!PC)
    {
        return;
    }

    const UPlayerViewModeSubsystem* Views =
        World->GetSubsystem<
            UPlayerViewModeSubsystem>();

    const bool bFirstPerson =
        Views &&
        Views->GetCurrentMode() ==
            EPlayerGameplayMode::FirstPerson;

    if (!bFirstPerson)
    {
        if (bWasFirstPerson)
        {
            ExitFirstPerson();
        }

        return;
    }

    if (!bWasFirstPerson)
    {
        EnterFirstPerson(PC);
    }

    // Rebind safely if the possessed character/mesh changed.
    BindCharacterMesh(PC);

    UpdateHandInput(
        PC,
        DeltaSeconds);

    if (PC->WasInputKeyJustPressed(
        EKeys::E))
    {
        TryExamine(PC);
    }

    if (AnalysisRemainingSeconds > 0.0f)
    {
        AnalysisRemainingSeconds =
            FMath::Max(
                0.0f,
                AnalysisRemainingSeconds -
                    DeltaSeconds);

        if (AnalysisRemainingSeconds <= 0.0f)
        {
            HideAnalysis();
        }
    }
}
