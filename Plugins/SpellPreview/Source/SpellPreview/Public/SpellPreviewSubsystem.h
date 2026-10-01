#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "SpellPreviewSubsystem.generated.h"

/**
 * Shared presentation-only spell preview.
 *
 * Live casting and the meditation Workbench both use this node. It observes
 * spell/realm state and renders it; it never owns or mutates gameplay values.
 */
UCLASS()
class SPELLPREVIEW_API USpellPreviewSubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Tick(float DeltaSeconds) override;
    virtual TStatId GetStatId() const override;
};