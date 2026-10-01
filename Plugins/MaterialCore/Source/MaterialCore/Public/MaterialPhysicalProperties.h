#pragma once
#include "CoreMinimal.h"
#include "MaterialPhysicalProperties.generated.h"

/** Generic material data. No impact, Earth, projectile, spell, mesh, or rendering knowledge belongs here. */
USTRUCT(BlueprintType)
struct MATERIALCORE_API FMaterialPhysicalProperties
{
    GENERATED_BODY()

    /** Density describes the substance, not a particular object's total mass. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Material|Physical", meta=(ClampMin="1.0"))
    float DensityKgPerM3 = 1600.0f;

    /** Surface resistance to penetration/cutting. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Material|Physical", meta=(ClampMin="0.0", ClampMax="1.0"))
    float Hardness = 0.25f;

    /** How strongly particles/parts of the material hold together. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Material|Physical", meta=(ClampMin="0.0", ClampMax="1.0"))
    float Cohesion = 0.35f;

    /** How readily the material keeps permanent deformation. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Material|Physical", meta=(ClampMin="0.0", ClampMax="1.0"))
    float Plasticity = 0.65f;

    /** Surface sliding resistance. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Material|Physical", meta=(ClampMin="0.0", ClampMax="2.0"))
    float Friction = 0.75f;

    /** Bounce/energy-return tendency. 0 = dead impact, 1 = highly elastic. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Material|Physical", meta=(ClampMin="0.0", ClampMax="1.0"))
    float Restitution = 0.08f;

    /** Energy absorption before structural failure. High toughness resists shattering. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Material|Physical", meta=(ClampMin="0.0", ClampMax="1.0"))
    float Toughness = 0.45f;

    /** Shape-holding stiffness. High rigidity transfers impact cleanly; low rigidity behaves more fluidly. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Material|Physical", meta=(ClampMin="0.0", ClampMax="1.0"))
    float Rigidity = 0.55f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Material|Physical", meta=(Units="Celsius"))
    float TemperatureC = 15.0f;
};
