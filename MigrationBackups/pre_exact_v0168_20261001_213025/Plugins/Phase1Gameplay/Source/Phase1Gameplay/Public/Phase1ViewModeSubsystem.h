#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Core/Phase1GameplayModeTypes.h"
#include "Phase1ViewModeSubsystem.generated.h"

class APlayerController;
class FPhase1ModeNode;

/**
 * Single coordinator for the four gameplay states.
 *
 * Up Arrow:   FirstPerson -> ThirdPerson -> TopDown -> FreeRoam
 * Down Arrow: FreeRoam -> TopDown -> ThirdPerson -> FirstPerson
 *
 * No mode directly references another mode and this plugin has no dependency on
 * magic, UI, characters/animations, landscapes, or external assets.
 */
UCLASS()
class PHASE1GAMEPLAY_API UPhase1ViewModeSubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual TStatId GetStatId() const override;

    UFUNCTION(BlueprintPure, Category = "Phase 1|Mode")
    EPhase1GameplayMode GetCurrentMode() const { return CurrentMode; }

    UFUNCTION(BlueprintPure, Category = "Phase 1|Mode")
    FPhase1ModeCapabilities GetCurrentCapabilities() const;

    bool GetGameplayCastRay(FVector& OutOrigin, FVector& OutDirection) const;
    bool GetFreeRoamInteractionPoint(FVector& OutPoint) const;

private:
    FPhase1ModeNode* FirstPersonNode = nullptr;
    FPhase1ModeNode* ThirdPersonNode = nullptr;
    FPhase1ModeNode* TopDownNode = nullptr;
    FPhase1ModeNode* FreeRoamNode = nullptr;

    EPhase1GameplayMode CurrentMode = EPhase1GameplayMode::ThirdPerson;
    bool bModeInitialized = false;

    FPhase1ModeNode* GetNode(EPhase1GameplayMode Mode) const;
    FPhase1ModeNode* GetActiveNode() const;

    void TransitionToMode(EPhase1GameplayMode NewMode, APlayerController* PC);
    void StepMode(int32 Direction, APlayerController* PC);
    void AnnounceMode() const;
};
