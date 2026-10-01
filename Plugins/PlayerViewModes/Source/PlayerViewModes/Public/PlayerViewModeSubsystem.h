#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Core/PlayerGameplayModeNode.h"
#include "Core/PlayerGameplayModeTypes.h"
#include "PlayerViewModeSubsystem.generated.h"

class APlayerController;

/**
 * Central mode coordinator.
 *
 * Up Arrow:
 * FirstPerson -> ThirdPerson -> TopDown -> FreeRoam
 *
 * Down Arrow:
 * FreeRoam -> TopDown -> ThirdPerson -> FirstPerson
 *
 * Modes never know about one another. They only implement the shared node contract.
 */
UCLASS()
class PLAYERVIEWMODES_API UPlayerViewModeSubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual TStatId GetStatId() const override;

    UFUNCTION(BlueprintPure, Category="Player Mode")
    EPlayerGameplayMode GetCurrentMode() const { return CurrentMode; }

    UFUNCTION(BlueprintPure, Category="Player Mode")
    bool IsTopDown() const { return CurrentMode == EPlayerGameplayMode::TopDown; }

    UFUNCTION(BlueprintPure, Category="Player Mode")
    FPlayerModeCapabilities GetCurrentCapabilities() const;

    /**
     * Generic gameplay aim contract.
     * TopDown and FreeRoam currently provide mouse-world-plane aim.
     * Other modes fall back to their normal camera aim in SpellExecution.
     */
    bool GetGameplayCastRay(FVector& OutOrigin, FVector& OutDirection) const;

    /** Compatibility contract retained for existing top-down callers. */
    bool GetTopDownCastRay(FVector& OutOrigin, FVector& OutDirection) const;

    /** Free-roam world interaction point, available after a successful RMB click. */
    bool GetFreeRoamInteractionPoint(FVector& OutPoint) const;

private:
    UPROPERTY(Transient)
    TObjectPtr<UPlayerGameplayModeNode> FirstPersonNode;

    UPROPERTY(Transient)
    TObjectPtr<UPlayerGameplayModeNode> ThirdPersonNode;

    UPROPERTY(Transient)
    TObjectPtr<UPlayerGameplayModeNode> TopDownNode;

    UPROPERTY(Transient)
    TObjectPtr<UPlayerGameplayModeNode> FreeRoamNode;

    EPlayerGameplayMode CurrentMode = EPlayerGameplayMode::ThirdPerson;
    bool bModeInitialized = false;
    bool bWasMeditating = false;

    UPlayerGameplayModeNode* GetNode(EPlayerGameplayMode Mode) const;
    UPlayerGameplayModeNode* GetActiveNode() const;
    void TransitionToMode(EPlayerGameplayMode NewMode, APlayerController* PC);
    void StepMode(int32 Direction, APlayerController* PC);
    void ApplyCapabilities();
    void AnnounceMode() const;
};
