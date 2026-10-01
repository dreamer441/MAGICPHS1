#pragma once

#include "CoreMinimal.h"
#include "MaterialPhysicalProperties.h"
#include "PhysicalBodyState.generated.h"

/**
 * Generic state of one physical body at a moment in time.
 *
 * Material describes WHAT it is made from.
 * Mass/volume/velocity describe THIS PARTICULAR body.
 * No collision response, Earth logic, spell logic, or rendering belongs here.
 */
USTRUCT(BlueprintType)
struct PHYSICALBODY_API FPhysicalBodyState
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Physical Body", meta=(ShowOnlyInnerProperties))
    FMaterialPhysicalProperties Material;

    /** Authoritative body mass. Use material density + volume only when a caller needs to derive it. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Physical Body", meta=(ClampMin="0.000001", Units="kg"))
    float MassKg = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Physical Body", meta=(ClampMin="0.000001"))
    float VolumeM3 = 0.01f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Physical Body")
    FVector VelocityCmS = FVector::ZeroVector;

    /** Approximate local contact radius used by generic impact geometry calculations. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Physical Body", meta=(ClampMin="0.1", Units="cm"))
    float ContactRadiusCm = 10.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Physical Body")
    bool bAffectedByGravity = true;
};
