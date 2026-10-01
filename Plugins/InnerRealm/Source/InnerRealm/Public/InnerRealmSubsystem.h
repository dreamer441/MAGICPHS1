#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/InputChord.h"
#include "Subsystems/WorldSubsystem.h"
#include "SpellDefinition.h"
#include "Layout/Visibility.h"
#include "InnerRealmSubsystem.generated.h"

class AActor;
class AInnerRealmActor;
class APawn;
class APlayerController;
class SWidget;
class USpellCreationSubsystem;
class USpellCastingBindingSubsystem;
class UWorldCodexSubsystem;
class USpellGraphSubsystem;
enum class ESpellLiveAction : uint8;

enum class EInnerRealmPage : uint8
{
    Spell,
    Codex,
    Canvas
};

/**
 * Meditation-state node.
 * Owns TAB switching, the fixed realm camera, frozen-player state and the
 * configuration UI. SpellCreation owns values; SpellCastingBindings owns
 * keyboard semantics; this node only presents/edit them together.
 */
UCLASS()
class INNERREALM_API UInnerRealmSubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Deinitialize() override;
    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override;

    UFUNCTION(BlueprintPure, Category="Inner Realm")
    bool IsActive() const { return bActive; }

    bool IsSpellWorkbenchVisible() const
    {
        return bActive &&
            (ActivePage == EInnerRealmPage::Spell ||
             ActivePage == EInnerRealmPage::Canvas);
    }

private:
    bool bActive = false;
    EInnerRealmPage ActivePage = EInnerRealmPage::Spell;
    FName SelectedCodexConcept = FName(TEXT("element.earth"));
    FGuid SelectedCanvasNode;
    TSet<FGuid> ExpandedCanvasNodes;

    /**
     * Rune Canvas palette navigation only.
     * Categories organize Sign selection but are NOT semantic graph nodes.
     */
    int32 SelectedCanvasPaletteCategory = INDEX_NONE;

    FName PendingCanvasElement = NAME_None;

    TSharedPtr<SWidget> ShellWidget;
    TSharedPtr<SWidget> EditorWidget;
    TSharedPtr<SWidget> PreviewFrameWidget;
    TSharedPtr<SWidget> CodexWidget;
    TSharedPtr<SWidget> CanvasWidget;
    TWeakObjectPtr<AInnerRealmActor> RealmActor;
    TWeakObjectPtr<AActor> PreviousViewTarget;

    TWeakObjectPtr<APawn> FrozenPawn;
    FTransform FrozenPawnTransform = FTransform::Identity;
    uint8 SavedMovementMode = 0;
    uint8 SavedCustomMovementMode = 0;
    bool bSavedCharacterMovement = false;

    void EnterRealm();
    void ExitRealm();
    AInnerRealmActor* GetOrCreateRealmActor();
    void ApplyPlayerInputState(bool bEntering);
    void CaptureAndFreezePlayer(APlayerController* PC);
    void RestoreFrozenPlayer();
    void PositionRealmNearPlayer(AInnerRealmActor* Realm, APlayerController* PC);
    void CreateEditorWidget();
    void RemoveEditorWidget();

    USpellCreationSubsystem* GetSpellCreation() const;
    USpellCastingBindingSubsystem* GetSpellBindings() const;
    UWorldCodexSubsystem* GetWorldCodex() const;
    USpellGraphSubsystem* GetSpellGraph() const;

    void RebuildCanvasWidget();

    FText GetSelectedCodexTitle() const;
    FText GetSelectedCodexSign() const;
    FText GetSelectedCodexDetails() const;

    FSpellDefinition ReadSpell() const;
    void WriteSpell(const FSpellDefinition& Spell);
    void SetShape(ESpellShape Shape);
    FText GetShapeName() const;

    FInputChord GetBindingChord(ESpellLiveAction Action) const;
    void HandleBindingSelected(ESpellLiveAction Action, const FInputChord& Chord);
    FText GetCurrentSpellSummary() const;

    float GetSpeedSlider() const;
    float GetDistanceSlider() const;
    float GetDensitySlider() const;
    float GetHardnessSlider() const;
    float GetToughnessSlider() const;
    float GetElasticitySlider() const;

    void SetSpeedSlider(float Value);
    void SetDistanceSlider(float Value);
    void SetDensitySlider(float Value);
    void SetHardnessSlider(float Value);
    void SetToughnessSlider(float Value);
    void SetElasticitySlider(float Value);

    float GetDimensionSlider(int32 Index) const;
    void SetDimensionSlider(int32 Index, float Value);
    EVisibility GetDimensionVisibility(int32 Index) const;
    FText GetDimensionLabel(int32 Index) const;
};
