#include "UI/InnerRealmShellUI.h"

#include "InnerRealmSubsystem.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

TSharedRef<SWidget> FInnerRealmShellUI::Build(
    const FInnerRealmShellCallbacks& Callbacks)
{
    const FSlateColor BlackText(FLinearColor::Black);

    auto MakePageButton =
        [Callbacks, BlackText](
            const EInnerRealmPage Page,
            const TCHAR* Label) -> TSharedRef<SWidget>
    {
        return SNew(SBox)
            .WidthOverride(190.0f)
            [
                SNew(SButton)
                .IsFocusable(false)
                .ContentPadding(FMargin(14.0f, 7.0f))
                .OnClicked_Lambda([Callbacks, Page]()
                {
                    if (Callbacks.SetActivePage)
                    {
                        Callbacks.SetActivePage(Page);
                    }
                    return FReply::Handled();
                })
                [
                    SNew(STextBlock)
                    .Justification(ETextJustify::Center)
                    .ColorAndOpacity(BlackText)
                    .Text_Lambda([Callbacks, Page, Label]()
                    {
                        const bool bSelected =
                            Callbacks.GetActivePage &&
                            Callbacks.GetActivePage() == Page;

                        return FText::FromString(
                            bSelected
                                ? FString::Printf(TEXT("[ %s ]"), Label)
                                : FString(Label));
                    })
                ]
            ];
    };

    auto MakeReadySlot =
        [Callbacks, BlackText](
            const int32 SlotIndex,
            const TCHAR* Label) -> TSharedRef<SWidget>
    {
        return SNew(SButton)
            .IsFocusable(false)
            .ContentPadding(FMargin(8.0f, 5.0f))
            .ToolTipText(FText::FromString(
                TEXT("Save the current constructed spell into this ready slot.")))
            .OnClicked_Lambda([Callbacks, SlotIndex]()
            {
                if (Callbacks.SavePreparedSlot)
                {
                    Callbacks.SavePreparedSlot(SlotIndex);
                }
                return FReply::Handled();
            })
            [
                SNew(STextBlock)
                .ColorAndOpacity(BlackText)
                .Text_Lambda([Callbacks, SlotIndex, Label]()
                {
                    const bool bOccupied =
                        Callbacks.IsPreparedSlotOccupied &&
                        Callbacks.IsPreparedSlotOccupied(SlotIndex);

                    return FText::FromString(
                        bOccupied
                            ? FString::Printf(TEXT("%s*"), Label)
                            : FString(Label));
                })
            ];
    };

    return SNew(SBox)
        .HAlign(HAlign_Center)
        .VAlign(VAlign_Top)
        [
            SNew(SBorder)
            .Padding(FMargin(10.0f, 7.0f))
            .BorderBackgroundColor(
                FLinearColor(0.92f, 0.90f, 0.84f, 0.99f))
            [
                SNew(SVerticalBox)

                + SVerticalBox::Slot()
                .AutoHeight()
                .HAlign(HAlign_Center)
                [
                    SNew(SHorizontalBox)

                    + SHorizontalBox::Slot()
                    .AutoWidth()
                    .Padding(4, 0)
                    [
                        MakePageButton(
                            EInnerRealmPage::Spell,
                            TEXT("SPELL MODIFIER"))
                    ]

                    + SHorizontalBox::Slot()
                    .AutoWidth()
                    .Padding(4, 0)
                    [
                        MakePageButton(
                            EInnerRealmPage::Codex,
                            TEXT("CODEX"))
                    ]

                    + SHorizontalBox::Slot()
                    .AutoWidth()
                    .Padding(4, 0)
                    [
                        MakePageButton(
                            EInnerRealmPage::Canvas,
                            TEXT("RUNE CANVAS"))
                    ]
                ]

                + SVerticalBox::Slot()
                .AutoHeight()
                .HAlign(HAlign_Center)
                .Padding(0, 6, 0, 2)
                [
                    SNew(STextBlock)
                    .ColorAndOpacity(BlackText)
                    .Text(FText::FromString(
                        TEXT("READY SPELLS  -  save current construction")))
                ]

                + SVerticalBox::Slot()
                .AutoHeight()
                .HAlign(HAlign_Center)
                [
                    SNew(SHorizontalBox)

                    + SHorizontalBox::Slot().AutoWidth().Padding(2)
                    [ MakeReadySlot(0, TEXT("1")) ]
                    + SHorizontalBox::Slot().AutoWidth().Padding(2)
                    [ MakeReadySlot(1, TEXT("2")) ]
                    + SHorizontalBox::Slot().AutoWidth().Padding(2)
                    [ MakeReadySlot(2, TEXT("3")) ]
                    + SHorizontalBox::Slot().AutoWidth().Padding(2)
                    [ MakeReadySlot(3, TEXT("4")) ]
                    + SHorizontalBox::Slot().AutoWidth().Padding(2)
                    [ MakeReadySlot(4, TEXT("5")) ]
                    + SHorizontalBox::Slot().AutoWidth().Padding(2)
                    [ MakeReadySlot(5, TEXT("6")) ]
                    + SHorizontalBox::Slot().AutoWidth().Padding(2)
                    [ MakeReadySlot(6, TEXT("7")) ]
                    + SHorizontalBox::Slot().AutoWidth().Padding(2)
                    [ MakeReadySlot(7, TEXT("8")) ]
                    + SHorizontalBox::Slot().AutoWidth().Padding(2)
                    [ MakeReadySlot(8, TEXT("9")) ]
                    + SHorizontalBox::Slot().AutoWidth().Padding(2)
                    [ MakeReadySlot(9, TEXT("0")) ]
                ]
            ]
        ];
}
