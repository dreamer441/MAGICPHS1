#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InnerRealmActor.generated.h"

class UCameraComponent;
class UPointLightComponent;
class USceneComponent;
class USkeletalMeshComponent;
class UStaticMeshComponent;

/**
 * Pure presentation actor for the meditation realm.
 *
 * v0.13 Workbench V1 adds a mannequin reference scene on the right side of
 * the meditation view. The mannequin is presentation-only: it is not the
 * gameplay pawn and it owns no spell logic.
 */
UCLASS()
class INNERREALM_API AInnerRealmActor : public AActor
{
    GENERATED_BODY()

public:
    AInnerRealmActor();

    /** Reference frame used by SpellPreview for the Workbench preview. */
    FTransform GetPreviewReferenceTransform() const;

private:
    UPROPERTY(VisibleAnywhere)
    TObjectPtr<USceneComponent> Root;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UCameraComponent> RealmCamera;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UStaticMeshComponent> BlankPlane;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UPointLightComponent> RealmLight;

    /** Presentation-only reference transform representing the preview caster. */
    UPROPERTY(VisibleAnywhere)
    TObjectPtr<USceneComponent> PreviewReference;

    /** Manny reference model used only for size / placement context. */
    UPROPERTY(VisibleAnywhere)
    TObjectPtr<USkeletalMeshComponent> PreviewPlayer;
};
