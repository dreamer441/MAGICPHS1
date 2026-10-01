#include "Modes/FirstPerson/FirstPersonModeNode.h"

#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "PlayerFirstPersonCamera.h"

FPlayerModeCapabilities UFirstPersonModeNode::GetCapabilities() const
{
    FPlayerModeCapabilities Caps;
    Caps.bCharacterMovement = true;
    Caps.bJump = true;
    Caps.bSprint = true;
    return Caps;
}

APlayerFirstPersonCamera* UFirstPersonModeNode::GetOrSpawnCamera(APlayerController* PC)
{
    if (Camera.IsValid())
    {
        return Camera.Get();
    }

    UWorld* World = PC ? PC->GetWorld() : nullptr;
    if (!World)
    {
        return nullptr;
    }

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    Camera = World->SpawnActor<APlayerFirstPersonCamera>(
        APlayerFirstPersonCamera::StaticClass(),
        FVector::ZeroVector,
        FRotator::ZeroRotator,
        Params);

    return Camera.Get();
}

void UFirstPersonModeNode::ApplyPresentation(APlayerController* PC)
{
    if (!PC)
    {
        return;
    }

    PC->bShowMouseCursor = false;
    PC->SetInputMode(FInputModeGameOnly());

    ACharacter* Character = Cast<ACharacter>(PC->GetPawn());
    APlayerFirstPersonCamera* CameraActor = GetOrSpawnCamera(PC);
    if (Character && CameraActor)
    {
        CameraActor->Follow(Character, PC);
        PC->SetViewTargetWithBlend(CameraActor, 0.20f);
    }
}

void UFirstPersonModeNode::Enter(APlayerController* PC)
{
    ACharacter* Character = PC ? Cast<ACharacter>(PC->GetPawn()) : nullptr;
    Sprint.Enter(Character);
    ApplyPresentation(PC);
}

void UFirstPersonModeNode::Exit(APlayerController* PC)
{
    Sprint.Exit();

    if (Camera.IsValid())
    {
        Camera->Destroy();
        Camera.Reset();
    }
}

void UFirstPersonModeNode::TickMode(APlayerController* PC, const float DeltaSeconds)
{
    Sprint.Tick(PC, DeltaSeconds);

    ACharacter* Character = PC ? Cast<ACharacter>(PC->GetPawn()) : nullptr;
    if (Character && Camera.IsValid())
    {
        Camera->Follow(Character, PC);
    }
}

void UFirstPersonModeNode::ResumeAfterExternalView(APlayerController* PC)
{
    ApplyPresentation(PC);
}
