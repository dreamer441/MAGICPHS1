#include "InnerRealmSubsystem.h"

#include "InnerRealmActor.h"
#include "SpellShapeMath.h"
#include "SpellParameterRanges.h"
#include "LiveSpellSessionSubsystem.h"
#include "SpellCreationSubsystem.h"
#include "SpellCastingBindingSubsystem.h"
#include "WorldCodexSubsystem.h"
#include "SpellGraphSubsystem.h"
#include "SpellLoadoutSubsystem.h"
#include "UI/InnerRealmShellUI.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PawnMovementComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SInputKeySelector.h"
#include "Widgets/Input/SSlider.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SConstraintCanvas.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Styling/CoreStyle.h"

void UInnerRealmSubsystem::Deinitialize()
{
    if (bActive)
    {
        ExitRealm();
    }
    RemoveEditorWidget();
    Super::Deinitialize();
}

void UInnerRealmSubsystem::Tick(float DeltaTime)
{
    UWorld* World = GetWorld();
    if (!World || !World->IsGameWorld())
    {
        return;
    }

    APlayerController* PC = World->GetFirstPlayerController();
    if (!PC)
    {
        return;
    }

    if (PC->WasInputKeyJustPressed(EKeys::Tab))
    {
        bActive ? ExitRealm() : EnterRealm();
    }
}

TStatId UInnerRealmSubsystem::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UInnerRealmSubsystem, STATGROUP_Tickables);
}

USpellCreationSubsystem* UInnerRealmSubsystem::GetSpellCreation() const
{
    UWorld* World = GetWorld();
    return World ? World->GetSubsystem<USpellCreationSubsystem>() : nullptr;
}

USpellCastingBindingSubsystem* UInnerRealmSubsystem::GetSpellBindings() const
{
    UWorld* World = GetWorld();
    return World ? World->GetSubsystem<USpellCastingBindingSubsystem>() : nullptr;
}

UWorldCodexSubsystem* UInnerRealmSubsystem::GetWorldCodex() const
{
    UWorld* World = GetWorld();
    return World ? World->GetSubsystem<UWorldCodexSubsystem>() : nullptr;
}

USpellGraphSubsystem* UInnerRealmSubsystem::GetSpellGraph() const
{
    UWorld* World = GetWorld();
    return World ? World->GetSubsystem<USpellGraphSubsystem>() : nullptr;
}

AInnerRealmActor* UInnerRealmSubsystem::GetOrCreateRealmActor()
{
    if (RealmActor.IsValid())
    {
        return RealmActor.Get();
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return nullptr;
    }

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    AInnerRealmActor* Actor = World->SpawnActor<AInnerRealmActor>(
        AInnerRealmActor::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, Params);
    if (Actor)
    {
        RealmActor = Actor;
    }
    return Actor;
}

void UInnerRealmSubsystem::EnterRealm()
{
    if (bActive)
    {
        return;
    }

    UWorld* World = GetWorld();
    APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
    AInnerRealmActor* Realm = GetOrCreateRealmActor();
    if (!PC || !Realm)
    {
        return;
    }

    bActive = true;
    PreviousViewTarget = PC->GetViewTarget();
    if (ULiveSpellSessionSubsystem* LiveSession = World->GetSubsystem<ULiveSpellSessionSubsystem>())
    {
        LiveSession->ResetSession();
    }
    if (USpellCastingBindingSubsystem* Bindings = GetSpellBindings())
    {
        Bindings->SetSuspended(true);
    }

    CaptureAndFreezePlayer(PC);
    PositionRealmNearPlayer(Realm, PC);


    ApplyPlayerInputState(true);
    PC->SetViewTargetWithBlend(Realm, 0.15f);
    CreateEditorWidget();
}

void UInnerRealmSubsystem::ExitRealm()
{
    if (!bActive)
    {
        return;
    }

    UWorld* World = GetWorld();
    APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;


    RemoveEditorWidget();
    if (PC)
    {
        AActor* ReturnTarget = PreviousViewTarget.IsValid()
            ? PreviousViewTarget.Get()
            : Cast<AActor>(PC->GetPawn());
        if (ReturnTarget)
        {
            PC->SetViewTargetWithBlend(ReturnTarget, 0.15f);
        }
    }

    PreviousViewTarget.Reset();
    bActive = false;
    if (USpellCastingBindingSubsystem* Bindings = GetSpellBindings())
    {
        Bindings->SetSuspended(false);
    }
    ApplyPlayerInputState(false);
    RestoreFrozenPlayer();
}

void UInnerRealmSubsystem::CaptureAndFreezePlayer(APlayerController* PC)
{
    FrozenPawn.Reset();
    bSavedCharacterMovement = false;

    APawn* Pawn = PC ? PC->GetPawn() : nullptr;
    if (!Pawn)
    {
        return;
    }

    FrozenPawn = Pawn;
    FrozenPawnTransform = Pawn->GetActorTransform();

    if (ACharacter* Character = Cast<ACharacter>(Pawn))
    {
        if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
        {
            SavedMovementMode = static_cast<uint8>(Movement->MovementMode);
            SavedCustomMovementMode = Movement->CustomMovementMode;
            bSavedCharacterMovement = true;
            Movement->StopMovementImmediately();
            Movement->DisableMovement();
        }
    }
    else if (UPawnMovementComponent* Movement = Pawn->GetMovementComponent())
    {
        Movement->StopMovementImmediately();
        Movement->Deactivate();
    }
}

void UInnerRealmSubsystem::RestoreFrozenPlayer()
{
    APawn* Pawn = FrozenPawn.Get();
    if (!Pawn)
    {
        FrozenPawn.Reset();
        bSavedCharacterMovement = false;
        return;
    }

    Pawn->SetActorTransform(FrozenPawnTransform, false, nullptr, ETeleportType::TeleportPhysics);

    if (bSavedCharacterMovement)
    {
        if (ACharacter* Character = Cast<ACharacter>(Pawn))
        {
            if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
            {
                Movement->SetMovementMode(static_cast<EMovementMode>(SavedMovementMode), SavedCustomMovementMode);
            }
        }
    }
    else if (UPawnMovementComponent* Movement = Pawn->GetMovementComponent())
    {
        Movement->Activate(true);
    }

    FrozenPawn.Reset();
    bSavedCharacterMovement = false;
}

void UInnerRealmSubsystem::PositionRealmNearPlayer(AInnerRealmActor* Realm, APlayerController* PC)
{
    if (!Realm)
    {
        return;
    }

    FVector Base = FVector::ZeroVector;
    if (PC && PC->GetPawn())
    {
        Base = PC->GetPawn()->GetActorLocation();
    }
    Realm->SetActorLocation(Base + FVector(0.0f, 0.0f, 10000.0f));
}

void UInnerRealmSubsystem::ApplyPlayerInputState(const bool bEntering)
{
    UWorld* World = GetWorld();
    APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
    if (!PC)
    {
        return;
    }

    PC->SetIgnoreMoveInput(bEntering);
    PC->SetIgnoreLookInput(bEntering);
    PC->bShowMouseCursor = bEntering;

    if (bEntering)
    {
        FInputModeGameAndUI Mode;
        Mode.SetHideCursorDuringCapture(false);
        Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
        PC->SetInputMode(Mode);
    }
    else
    {
        PC->SetInputMode(FInputModeGameOnly());
        PC->bShowMouseCursor = false;
    }
}

