#pragma once

#include "CoreMinimal.h"
#include "Phase1GameplayModeTypes.generated.h"

UENUM(BlueprintType)
enum class EPhase1GameplayMode : uint8
{
    FirstPerson = 0 UMETA(DisplayName = "First Person"),
    ThirdPerson = 1 UMETA(DisplayName = "Third Person"),
    TopDown = 2 UMETA(DisplayName = "Top Down"),
    FreeRoam = 3 UMETA(DisplayName = "Free Roam")
};

USTRUCT(BlueprintType)
struct PHASE1GAMEPLAY_API FPhase1ModeCapabilities
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Phase 1")
    bool bCanCast = false;

    UPROPERTY(BlueprintReadOnly, Category = "Phase 1")
    bool bLiveConstruction = false;

    UPROPERTY(BlueprintReadOnly, Category = "Phase 1")
    bool bUsesWorldCursor = false;

    UPROPERTY(BlueprintReadOnly, Category = "Phase 1")
    bool bDetachedCamera = false;
};
