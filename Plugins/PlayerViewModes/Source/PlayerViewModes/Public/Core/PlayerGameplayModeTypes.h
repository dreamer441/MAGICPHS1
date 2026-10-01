#pragma once

#include "CoreMinimal.h"
#include "PlayerGameplayModeTypes.generated.h"

UENUM(BlueprintType)
enum class EPlayerGameplayMode : uint8
{
    FirstPerson UMETA(DisplayName="First Person"),
    ThirdPerson UMETA(DisplayName="Third Person"),
    TopDown     UMETA(DisplayName="Top Down"),
    FreeRoam    UMETA(DisplayName="Free Roam")
};

USTRUCT(BlueprintType)
struct PLAYERVIEWMODES_API FPlayerModeCapabilities
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="Player Mode")
    bool bCharacterMovement = false;

    UPROPERTY(BlueprintReadOnly, Category="Player Mode")
    bool bJump = false;

    UPROPERTY(BlueprintReadOnly, Category="Player Mode")
    bool bSprint = false;

    UPROPERTY(BlueprintReadOnly, Category="Player Mode")
    bool bLiveCasting = false;

    UPROPERTY(BlueprintReadOnly, Category="Player Mode")
    bool bSpellCreation = false;

    UPROPERTY(BlueprintReadOnly, Category="Player Mode")
    bool bWorldInteraction = false;

    UPROPERTY(BlueprintReadOnly, Category="Player Mode")
    bool bDetachedCamera = false;
};
