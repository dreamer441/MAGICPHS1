#pragma once

#include "CoreMinimal.h"
#include "SpellDefinition.h"

class AEarthSpellBody;
class APlayerController;
class UWorld;

/** Earth-only realization entry point used by the generic spell executor. */
struct EARTHMAGIC_API FEarthSpellSpawner
{
    static AEarthSpellBody* SpawnAndLaunch(
        UWorld* World,
        APlayerController* Controller,
        const FResolvedSpell& Spell,
        const FVector& SpawnLocation,
        const FRotator& SpawnRotation,
        const FVector& LaunchDirection);
};
