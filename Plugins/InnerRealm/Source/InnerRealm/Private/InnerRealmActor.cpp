#include "InnerRealmActor.h"

#include "Camera/CameraComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

AInnerRealmActor::AInnerRealmActor()
{
    PrimaryActorTick.bCanEverTick = false;

    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);

    RealmCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("RealmCamera"));
    RealmCamera->SetupAttachment(Root);
    RealmCamera->SetRelativeLocation(FVector::ZeroVector);
    RealmCamera->SetRelativeRotation(FRotator::ZeroRotator);
    RealmCamera->FieldOfView = 60.0f;

    // Keep the blank background behind the new Workbench reference scene.
    BlankPlane = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BlankPlane"));
    BlankPlane->SetupAttachment(Root);
    BlankPlane->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    BlankPlane->SetCastShadow(false);
    BlankPlane->SetRelativeLocation(FVector(1450.0f, 0.0f, 0.0f));
    BlankPlane->SetRelativeRotation(FRotator(90.0f, 0.0f, 0.0f));
    BlankPlane->SetRelativeScale3D(FVector(60.0f, 60.0f, 60.0f));

    static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneMesh(TEXT("/Engine/BasicShapes/Plane.Plane"));
    if (PlaneMesh.Succeeded())
    {
        BlankPlane->SetStaticMesh(PlaneMesh.Object);
    }

    static ConstructorHelpers::FObjectFinder<UMaterialInterface> PlaneMaterial(
        TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    if (PlaneMaterial.Succeeded())
    {
        BlankPlane->SetMaterial(0, PlaneMaterial.Object);
    }

    /*
     * Workbench reference scene:
     * - Camera looks along +X.
     * - Reference sits on the right side of the frame.
     * - Reference forward is -Y, so stored spell distance is visible sideways
     *   relative to the mannequin instead of disappearing into camera depth.
     */
    PreviewReference = CreateDefaultSubobject<USceneComponent>(TEXT("PreviewReference"));
    PreviewReference->SetupAttachment(Root);
    PreviewReference->SetRelativeLocation(FVector(900.0f, 520.0f, 0.0f));
    PreviewReference->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));

    PreviewPlayer = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("PreviewPlayer"));
    PreviewPlayer->SetupAttachment(PreviewReference);
    PreviewPlayer->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    PreviewPlayer->SetGenerateOverlapEvents(false);
    PreviewPlayer->SetCastShadow(false);
    PreviewPlayer->SetRelativeLocation(FVector(0.0f, 0.0f, -89.0f));

    // Manny assets are already part of the current project.
    // The -90 yaw is the standard mannequin mesh correction relative to the
    // reference/capsule forward axis.
    PreviewPlayer->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));

    static ConstructorHelpers::FObjectFinder<USkeletalMesh> MannyMesh(
        TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple"));
    if (MannyMesh.Succeeded())
    {
        PreviewPlayer->SetSkeletalMesh(MannyMesh.Object);
    }

    RealmLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("RealmLight"));
    RealmLight->SetupAttachment(Root);
    RealmLight->SetRelativeLocation(FVector(850.0f, 250.0f, 350.0f));
    RealmLight->SetIntensity(14000.0f);
    RealmLight->SetAttenuationRadius(3500.0f);
    RealmLight->SetCastShadows(false);
}

FTransform AInnerRealmActor::GetPreviewReferenceTransform() const
{
    return PreviewReference ? PreviewReference->GetComponentTransform() : GetActorTransform();
}
