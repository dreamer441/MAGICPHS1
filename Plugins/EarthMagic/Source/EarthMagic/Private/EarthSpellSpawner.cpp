#include "EarthSpellSpawner.h"

#include "EarthSpellBody.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

AEarthSpellBody* FEarthSpellSpawner::SpawnAndLaunch(
    UWorld* World,
    APlayerController* Controller,
    const FResolvedSpell& Spell,
    const FVector& SpawnLocation,
    const FRotator& SpawnRotation,
    const FVector& LaunchDirection)
{
    if (!World || !Controller)
    {
        return nullptr;
    }

    FActorSpawnParameters Parameters;
    Parameters.Owner = Controller;
    Parameters.Instigator = Controller->GetPawn();
    Parameters.SpawnCollisionHandlingOverride =
        ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    AEarthSpellBody* Body = World->SpawnActor<AEarthSpellBody>(
        AEarthSpellBody::StaticClass(), SpawnLocation, SpawnRotation, Parameters);
    if (!Body)
    {
        return nullptr;
    }

    Body->ConfigureResolvedSpell(Spell);
    Body->Launch(LaunchDirection);
    return Body;
}
