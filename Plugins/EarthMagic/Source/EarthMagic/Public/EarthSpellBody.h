#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ImpactProvider.h"
#include "ImpactReceiver.h"
#include "EarthSpellDefinition.h"
#include "SpellDefinition.h"
#include "EarthSpellBody.generated.h"

class UMaterialInterface;
class UPhysicalMaterial;
class UStaticMeshComponent;
class UDynamicMeshComponent;
class UEarthMaterialComponent;
class UEarthSubtractionComponent;

/**
 * The runtime adapter between a constructed Earth spell and our existing
 * material/impact/geometry contracts. No impact equations live in this actor.
 *
 * Moving state: Chaos uses a simple engine mesh collision proxy. The real
 * editable mesh is the visible surface after the first successful impact.
 * Once a damaged body is asleep, its editable mesh becomes exact static
 * collision; we never simulate a dynamic triangle mesh as a rigid body.
 */
UCLASS()
class EARTHMAGIC_API AEarthSpellBody : public AActor, public IImpactProvider, public IImpactReceiver
{
    GENERATED_BODY()

public:
    AEarthSpellBody();
    virtual void Tick(float DeltaSeconds) override;

    UFUNCTION(BlueprintCallable, Category="Earth Magic")
    void Configure(const FEarthSpellDefinition& InSpell);

    /** Generic execution entry point; the reflected Earth spell remains a compatibility shadow. */
    void ConfigureResolvedSpell(const FResolvedSpell& InSpell);

    UFUNCTION(BlueprintCallable, Category="Earth Magic")
    void Launch(const FVector& Direction);

    UFUNCTION(BlueprintPure, Category="Earth Magic")
    float GetBodyVolumeM3() const;

    UFUNCTION(BlueprintPure, Category="Earth Magic")
    float GetBodyMassKg() const;

    UFUNCTION(BlueprintPure, Category="Earth Magic")
    float GetBodyDensityKgPerM3() const;

    /**
     * Read-only runtime construction snapshot.
     * Added as a neutral inspection/debug contract; callers may read the
     * realized spell data but cannot mutate EarthSpellBody internals.
     */
    const FResolvedSpell& GetRuntimeSpell() const
    {
        return RuntimeSpell;
    }

    virtual bool BuildImpactRequest_Implementation(const FHitResult& Hit, FImpactRequest& OutRequest) const override;
    virtual bool ReceiveImpact_Implementation(const FImpactRequest& Request, FImpactResult& OutResult) override;

private:
    /** Moving, convex Chaos proxy. Also blocks the Pawn channel. */
    UPROPERTY(VisibleAnywhere, Category="Earth Magic|Nodes")
    TObjectPtr<UStaticMeshComponent> BodyMesh;

    /** Editable rendered mesh. While moving, it does not own collision. */
    UPROPERTY(VisibleAnywhere, Category="Earth Magic|Nodes")
    TObjectPtr<UDynamicMeshComponent> EditableMesh;

    /** Same Earth material state component used by the original Earth block. */
    UPROPERTY(VisibleAnywhere, Category="Earth Magic|Nodes")
    TObjectPtr<UEarthMaterialComponent> MaterialState;

    /** Same boolean subtraction adapter used by the original Earth block. */
    UPROPERTY(VisibleAnywhere, Category="Earth Magic|Nodes")
    TObjectPtr<UEarthSubtractionComponent> Subtraction;

    UPROPERTY(EditAnywhere, Category="Earth Magic|Impact")
    FImpactSolverTuning ImpactTuning;

    UPROPERTY(EditAnywhere, Category="Earth Magic")
    FEarthSpellDefinition Spell;

    /** Runtime source of truth for generic geometry, material, and body state. */
    FResolvedSpell RuntimeSpell;

    UPROPERTY(Transient)
    TObjectPtr<UPhysicalMaterial> RuntimePhysicalMaterial;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInterface> EarthVisualMaterial;

    /** Surviving body material volume, independent of its outer shape/bounds. */
    float SolidVolumeM3 = 0.0f;
    bool bHasEditableMesh = false;
    bool bUsesExactSettledCollision = false;
    float SettledSeconds = 0.0f;
    FVector LastFreeVelocityCmS = FVector::ZeroVector;
    float ImpactCooldownRemaining = 0.0f;

    void RebuildBodyGeometry();
    void RefreshPhysicalMaterial();
    void SyncEditableMeshScale();
    void EnsureEditableMesh();
    void ScaleEditableMesh(float LinearFactor);
    void RefreshMaterialState();
    void TryActivateExactSettledCollision(float DeltaSeconds);
    FVector GetIncomingVelocityCmS() const;
    void ScaleBodyAfterDamage(float RemainingVolumeFraction);

    UFUNCTION()
    void HandleBodyHit(
        UPrimitiveComponent* HitComponent,
        AActor* OtherActor,
        UPrimitiveComponent* OtherComponent,
        FVector NormalImpulse,
        const FHitResult& Hit);

    void ApplySourceOutcome(const FImpactResult& Result, const FHitResult& Hit, bool bGeometryChanged);
};
