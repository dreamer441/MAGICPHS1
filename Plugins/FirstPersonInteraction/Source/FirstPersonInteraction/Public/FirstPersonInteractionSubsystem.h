#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Widgets/SWidget.h"
#include "FirstPersonInteractionSubsystem.generated.h"

class APlayerController;
class USkeletalMeshComponent;

/**
 * State-1 interaction coordinator.
 *
 * V016.6 replaces the floating proxy blocks with direct procedural control of
 * Quinn's actual arm skeleton:
 *
 *   upperarm_l -> lowerarm_l -> hand_l
 *   upperarm_r -> lowerarm_r -> hand_r
 *
 * LMB activates the left arm.
 * RMB activates the right arm.
 * Mouse movement changes angular hand targets on a shoulder-centered reach
 * surface. The arm pose is solved after the normal animation pose finalizes,
 * so locomotion / jump animation can keep running underneath.
 *
 * Examination remains separate and unchanged:
 * E traces through the center of the first-person view and reports current
 * Codex-backed properties.
 */
UCLASS()
class FIRSTPERSONINTERACTION_API UFirstPersonInteractionSubsystem
    : public UTickableWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Deinitialize() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual TStatId GetStatId() const override;

    /** Runtime world positions for future weapon/tool trajectory consumers. */
    UFUNCTION(BlueprintPure, Category="First Person|Hand")
    FVector GetLeftHandWorldLocation() const
    {
        return LeftHandWorldLocation;
    }

    UFUNCTION(BlueprintPure, Category="First Person|Hand")
    FVector GetRightHandWorldLocation() const
    {
        return RightHandWorldLocation;
    }

    UFUNCTION(BlueprintPure, Category="First Person|Hand")
    FVector GetLeftHandVelocityCmS() const
    {
        return LeftHandVelocityCmS;
    }

    UFUNCTION(BlueprintPure, Category="First Person|Hand")
    FVector GetRightHandVelocityCmS() const
    {
        return RightHandVelocityCmS;
    }

private:
    bool bWasFirstPerson = false;

    // Angular offsets from each arm's forward/down resting direction.
    // X = horizontal arc around the shoulder.
    // Y = vertical arc around the shoulder.
    FVector2D LeftHandAnglesDeg = FVector2D::ZeroVector;
    FVector2D RightHandAnglesDeg = FVector2D::ZeroVector;

    // 0 = untouched animation pose, 1 = full procedural hand target.
    // These blend the real Quinn arms into/out of State-1 control so releasing
    // a mouse button returns the arm to ABP_Unarmed instead of leaving a
    // procedural pose behind.
    float LeftHandIKAlpha = 0.0f;
    float RightHandIKAlpha = 0.0f;

    // SetIgnoreLookInput is stacked by Unreal. We own exactly one look-only lease.
    bool bOwnsHandLookLock = false;
    TWeakObjectPtr<APlayerController> LookLockController;

    // Quinn skeletal mesh hook.
    TWeakObjectPtr<USkeletalMeshComponent> BoundMesh;
    TWeakObjectPtr<APlayerController> BoundController;
    FDelegateHandle BoneTransformsFinalizedHandle;

    int32 LeftUpperArmIndex = INDEX_NONE;
    int32 LeftLowerArmIndex = INDEX_NONE;
    int32 LeftHandIndex = INDEX_NONE;

    int32 RightUpperArmIndex = INDEX_NONE;
    int32 RightLowerArmIndex = INDEX_NONE;
    int32 RightHandIndex = INDEX_NONE;

    bool bArmBonesValid = false;
    bool bApplyingArmPose = false;
    bool bBoneStatusReported = false;

    FVector LeftHandWorldLocation = FVector::ZeroVector;
    FVector RightHandWorldLocation = FVector::ZeroVector;
    FVector LeftHandVelocityCmS = FVector::ZeroVector;
    FVector RightHandVelocityCmS = FVector::ZeroVector;
    bool bHavePreviousHandLocations = false;

    TSharedPtr<SWidget> AnalysisWidget;
    float AnalysisRemainingSeconds = 0.0f;

    void EnterFirstPerson(APlayerController* PC);
    void ExitFirstPerson();

    void BindCharacterMesh(APlayerController* PC);
    void UnbindCharacterMesh();
    bool ResolveArmBones();

    void UpdateHandInput(
        APlayerController* PC,
        float DeltaSeconds);

    void AcquireLookLock(APlayerController* PC);
    void ReleaseLookLock();

    /** Runs after Quinn's normal animation pose has been finalized. */
    void OnBoneTransformsFinalized();

    void ApplyArmIK(
        bool bLeftArm,
        TArray<FTransform>& ComponentSpaceTransforms);

    void RotateBoneSubtree(
        int32 AncestorBoneIndex,
        const FVector& PivotComponentSpace,
        const FQuat& DeltaRotation,
        TArray<FTransform>& ComponentSpaceTransforms) const;

    bool IsBoneDescendantOf(
        int32 BoneIndex,
        int32 AncestorBoneIndex) const;

    void UpdateHandTrajectoryState(
        const TArray<FTransform>& ComponentSpaceTransforms);

    void TryExamine(APlayerController* PC);

    bool BuildAnalysisText(
        AActor* Target,
        FString& OutText) const;

    void ShowAnalysis(const FString& Text);
    void HideAnalysis();
};
