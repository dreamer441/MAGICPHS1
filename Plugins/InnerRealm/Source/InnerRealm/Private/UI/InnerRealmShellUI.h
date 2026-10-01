#pragma once

#include "CoreMinimal.h"
#include "Widgets/SWidget.h"

enum class EInnerRealmPage : uint8;

/**
 * UI-only callbacks supplied by UInnerRealmSubsystem.
 *
 * The shell owns no gameplay state. It only presents:
 *   - page navigation
 *   - the shared prepared-spell slot bar
 *
 * Keeping these controls in one top-level widget prevents page Z-order from
 * deciding whether slot buttons are clickable.
 */
struct FInnerRealmShellCallbacks
{
    TFunction<EInnerRealmPage()> GetActivePage;
    TFunction<void(EInnerRealmPage)> SetActivePage;

    TFunction<void(int32)> SavePreparedSlot;
    TFunction<bool(int32)> IsPreparedSlotOccupied;
};

class FInnerRealmShellUI
{
public:
    static TSharedRef<SWidget> Build(
        const FInnerRealmShellCallbacks& Callbacks);
};
