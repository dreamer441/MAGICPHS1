#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FirstPersonHandProxy.generated.h"

class USceneComponent;
class UStaticMeshComponent;

UENUM(BlueprintType)
enum class EFirstPersonHandSide : uint8
{
    Left,
    Right
};

/**
 * Lightweight physical-hand presentation node for State 1.
 *
 * It deliberately does NOT contain combat equations, weapon logic, grabbing,
 * crafting or damage. Those systems can later attach equipment to GripPoint
 * and consume the hand trajectory / velocity exposed here.
 */
UCLASS(NotBlueprintable)
class FIRSTPERSONINTERACTION_API AFirstPersonHandProxy : public AActor
{
    GENERATED_BODY()

public:
    AFirstPersonHandProxy();

    void InitializeHand(EFirstPersonHandSide InSide);

    void SetHandTransform(
        const FVector& WorldLocation,
        const FRotator& WorldRotation,
        float DeltaSeconds);

    UFUNCTION(BlueprintPure, Category="First Person|Hand")
    EFirstPersonHandSide GetHandSide() const { return Side; }

    UFUNCTION(BlueprintPure, Category="First Person|Hand")
    FVector GetLinearVelocityCmS() const { return LinearVelocityCmS; }

    UFUNCTION(BlueprintPure, Category="First Person|Hand")
    USceneComponent* GetGripPoint() const { return GripPoint; }

private:
    UPROPERTY(VisibleAnywhere, Category="First Person|Hand")
    TObjectPtr<USceneComponent> HandRoot;

    UPROPERTY(VisibleAnywhere, Category="First Person|Hand")
    TObjectPtr<UStaticMeshComponent> HandMesh;

    /** Future swords, shields, tools, crafting implements etc. attach here. */
    UPROPERTY(VisibleAnywhere, Category="First Person|Hand")
    TObjectPtr<USceneComponent> GripPoint;

    UPROPERTY(VisibleAnywhere, Category="First Person|Hand")
    EFirstPersonHandSide Side = EFirstPersonHandSide::Left;

    FVector PreviousLocation = FVector::ZeroVector;
    FVector LinearVelocityCmS = FVector::ZeroVector;
    bool bHavePreviousLocation = false;
};
