#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"

/**
 * Owns exactly one paired SetIgnoreMoveInput/SetIgnoreLookInput lease.
 * Unreal stacks these calls, so this helper prevents accidental double-acquire.
 */
class FControllerInputLock
{
public:
    void Acquire(APlayerController* PC)
    {
        if (!PC)
        {
            return;
        }

        if (bOwned && Owner.Get() != PC)
        {
            Release();
        }

        if (!bOwned)
        {
            PC->SetIgnoreMoveInput(true);
            PC->SetIgnoreLookInput(true);
            Owner = PC;
            bOwned = true;
        }
    }

    void Release()
    {
        if (!bOwned)
        {
            return;
        }

        if (APlayerController* PC = Owner.Get())
        {
            PC->SetIgnoreMoveInput(false);
            PC->SetIgnoreLookInput(false);
        }

        Owner.Reset();
        bOwned = false;
    }

    bool IsOwned() const { return bOwned; }

private:
    TWeakObjectPtr<APlayerController> Owner;
    bool bOwned = false;
};
