#include "InnerRealmSubsystem.h"

#include "SpellCastingBindingSubsystem.h"
#include "SpellCreationSubsystem.h"
#include "SpellGraphSubsystem.h"
#include "SpellLoadoutSubsystem.h"
#include "SpellParameterRanges.h"
#include "SpellShapeMath.h"
#include "WorldCodexSubsystem.h"
#include "WorldCodexTypes.h"
#include "UI/InnerRealmShellUI.h"

#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "InputCoreTypes.h"

#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SInputKeySelector.h"
#include "Widgets/Input/SSlider.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Styling/CoreStyle.h"

void UInnerRealmSubsystem::CreateEditorWidget()
{
    if (EditorWidget.IsValid() || !GEngine || !GEngine->GameViewport)
    {
        return;
    }

    static FTextBlockStyle BlackKeyTextStyle = FCoreStyle::Get().GetWidgetStyle<FTextBlockStyle>("NormalText");
    BlackKeyTextStyle.SetColorAndOpacity(FSlateColor(FLinearColor::Black));
    const FSlateColor BlackText(FLinearColor::Black);

    auto MakeBindSelector = [this](const ESpellLiveAction Action) -> TSharedRef<SWidget>
    {
        return SNew(SBox)
            .WidthOverride(92.0f)
            [
                SNew(SInputKeySelector)
                .SelectedKey_Lambda([this, Action]() { return GetBindingChord(Action); })
                .OnKeySelected_Lambda([this, Action](const FInputChord& Chord)
                {
                    HandleBindingSelected(Action, Chord);
                })
                .AllowGamepadKeys(false)
                .AllowModifierKeys(false)
                .EscapeCancelsSelection(true)
                .KeySelectionText(FText::FromString(TEXT("key")))
                .NoKeySpecifiedText(FText::FromString(TEXT("+")))
                .TextStyle(&BlackKeyTextStyle)
                .Margin(FMargin(6.0f, 3.0f))
            ];
    };

    auto MakeShapeControl = [this, &MakeBindSelector, BlackText](
        const ESpellLiveAction Action,
        const ESpellShape Shape,
        const TCHAR* Label) -> TSharedRef<SWidget>
    {
        return SNew(SHorizontalBox)
            + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 2, 8, 2)
            [ MakeBindSelector(Action) ]
            + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 2)
            [
                SNew(SButton)
                .IsFocusable(false)
                .ContentPadding(FMargin(12.0f, 6.0f))
                .OnClicked_Lambda([this, Shape]()
                {
                    SetShape(Shape);
                    return FReply::Handled();
                })
                [
                    SNew(STextBlock)
                    .ColorAndOpacity(BlackText)
                    .Text_Lambda([this, Shape, Label]()
                    {
                        return FText::FromString(ReadSpell().Shape == Shape
                            ? FString::Printf(TEXT("[ %s ]"), Label)
                            : FString(Label));
                    })
                ]
            ];
    };

    auto MakeOrientationControl = [this, BlackText](
        const ESpellOrientationAxis Orientation,
        const TCHAR* Label) -> TSharedRef<SWidget>
    {
        return SNew(SButton)
            .IsFocusable(false)
            .ContentPadding(FMargin(12.0f, 6.0f))
            .OnClicked_Lambda([this, Orientation]()
            {
                FSpellDefinition Spell = ReadSpell();
                Spell.Orientation = Orientation;
                WriteSpell(Spell);
                return FReply::Handled();
            })
            [
                SNew(STextBlock)
                .ColorAndOpacity(BlackText)
                .Text_Lambda([this, Orientation, Label]()
                {
                    return FText::FromString(
                        ReadSpell().Orientation == Orientation
                            ? FString::Printf(TEXT("[ %s ]"), Label)
                            : FString(Label));
                })
            ];
    };

    auto MakeDimension = [this, &MakeBindSelector, BlackText](
        const ESpellShape Shape,
        const int32 Index,
        const ESpellLiveAction Action) -> TSharedRef<SWidget>
    {
        return SNew(SVerticalBox)
            .Visibility_Lambda([this, Shape]()
            {
                return ReadSpell().Shape == Shape ? EVisibility::Visible : EVisibility::Collapsed;
            })
            + SVerticalBox::Slot().AutoHeight().Padding(0, 3)
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 8, 0)
                [ MakeBindSelector(Action) ]
                + SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
                [
                    SNew(STextBlock)
                    .ColorAndOpacity(BlackText)
                    .Text_Lambda([this, Index]() { return GetDimensionLabel(Index); })
                ]
            ]
            + SVerticalBox::Slot().AutoHeight().Padding(100, 0, 0, 9)
            [
                SNew(SSlider)
                .IsFocusable(false)
                .Value_Lambda([this, Index]() { return GetDimensionSlider(Index); })
                .OnValueChanged_Lambda([this, Index](float V) { SetDimensionSlider(Index, V); })
            ];
    };

    auto Section = [BlackText](const TCHAR* Title) -> TSharedRef<SWidget>
    {
        return SNew(SBorder)
            .Padding(FMargin(8.0f, 6.0f))
            .BorderBackgroundColor(FLinearColor(0.78f, 0.76f, 0.70f, 0.96f))
            [
                SNew(STextBlock)
                .ColorAndOpacity(BlackText)
                .Text(FText::FromString(Title))
            ];
    };

    auto MakeArrangementControl = [this, BlackText](
        const ESpellArrangement Arrangement,
        const TCHAR* Label) -> TSharedRef<SWidget>
    {
        return SNew(SButton)
            .IsFocusable(false)
            .OnClicked_Lambda([this, Arrangement]()
            {
                FSpellDefinition Spell = ReadSpell();
                Spell.Pattern.Arrangement = Arrangement;
                WriteSpell(Spell);
                return FReply::Handled();
            })
            [
                SNew(STextBlock)
                .ColorAndOpacity(BlackText)
                .Text_Lambda([this, Arrangement, Label]()
                {
                    return FText::FromString(ReadSpell().Pattern.Arrangement == Arrangement
                        ? FString::Printf(TEXT("[ %s ]"), Label)
                        : FString(Label));
                })
            ];
    };

    auto MakePatternAxisControl = [this, BlackText](
        const ESpellPatternAxis Axis,
        const TCHAR* Label) -> TSharedRef<SWidget>
    {
        return SNew(SButton)
            .IsFocusable(false)
            .OnClicked_Lambda([this, Axis]()
            {
                FSpellDefinition Spell = ReadSpell();
                Spell.Pattern.LineAxis = Axis;
                WriteSpell(Spell);
                return FReply::Handled();
            })
            [
                SNew(STextBlock)
                .ColorAndOpacity(BlackText)
                .Text_Lambda([this, Axis, Label]()
                {
                    return FText::FromString(ReadSpell().Pattern.LineAxis == Axis
                        ? FString::Printf(TEXT("[ %s ]"), Label)
                        : FString(Label));
                })
            ];
    };

    auto MakePatternOrientationControl = [this, BlackText](
        const ESpellPatternOrientation Mode,
        const TCHAR* Label) -> TSharedRef<SWidget>
    {
        return SNew(SButton)
            .IsFocusable(false)
            .OnClicked_Lambda([this, Mode]()
            {
                FSpellDefinition Spell = ReadSpell();
                Spell.Pattern.InstanceOrientation = Mode;
                WriteSpell(Spell);
                return FReply::Handled();
            })
            [
                SNew(STextBlock)
                .ColorAndOpacity(BlackText)
                .Text_Lambda([this, Mode, Label]()
                {
                    return FText::FromString(
                        ReadSpell().Pattern.InstanceOrientation == Mode
                            ? FString::Printf(TEXT("[ %s ]"), Label)
                            : FString(Label));
                })
            ];
    };

    auto MakeMotionDirectionControl = [this, BlackText](
        const ESpellMotionDirection Direction,
        const TCHAR* Label) -> TSharedRef<SWidget>
    {
        return SNew(SButton)
            .IsFocusable(false)
            .OnClicked_Lambda([this, Direction]()
            {
                FSpellDefinition Spell = ReadSpell();
                Spell.MotionDirection = Direction;
                WriteSpell(Spell);
                return FReply::Handled();
            })
            [
                SNew(STextBlock)
                .ColorAndOpacity(BlackText)
                .Text_Lambda([this, Direction, Label]()
                {
                    if (ReadSpell().MotionDirection == Direction)
                    {
                        return FText::FromString(
                            FString::Printf(TEXT("[ %s ]"), Label));
                    }
                    return FText::FromString(Label);
                })
            ];
    };


    EditorWidget = SNew(SBox)
        .Visibility_Lambda([this]()
        {
            return ActivePage == EInnerRealmPage::Spell
                ? EVisibility::SelfHitTestInvisible
                : EVisibility::Collapsed;
        })
        .HAlign(HAlign_Left)
        .VAlign(VAlign_Center)
        [
            SNew(SBorder)
            .Padding(FMargin(24.0f))
            .BorderBackgroundColor(FLinearColor(0.92f, 0.90f, 0.84f, 0.96f))
            [
                SNew(SBox)
                .WidthOverride(820.0f)
                .HeightOverride(760.0f)
                [
                    SNew(SScrollBox)
                    + SScrollBox::Slot()
                    [
                        SNew(SVerticalBox)

                        + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 0, 0, 4)
                        [ SNew(STextBlock).ColorAndOpacity(BlackText).Text(FText::FromString(TEXT("MEDITATION REALM / EARTH SPELL CONSTRUCTION"))) ]

                        + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 0, 0, 8)
                        [ SNew(STextBlock).ColorAndOpacity(BlackText).Text(FText::FromString(TEXT("Sliders are persistent defaults. Key boxes configure temporary live overrides. Press the same assigned key again to clear it."))) ]

                        + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 7)
                        [ SNew(STextBlock).ColorAndOpacity(BlackText).Text(FText::FromString(TEXT("LIVE HOLD: unpressed = TAB default; press starts at minimum; 1 s small, 2 s medium, 3 s maximum."))) ]

                        + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 14)
                        [ SNew(STextBlock).ColorAndOpacity(BlackText).Text_Lambda([this]() { return GetCurrentSpellSummary(); }) ]

                        + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 3)
                        [ Section(TEXT("ELEMENT")) ]
                        + SVerticalBox::Slot().AutoHeight().Padding(0, 4, 0, 8)
                        [
                            SNew(SHorizontalBox)
                            + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 8, 0)
                            [ MakeBindSelector(ESpellLiveAction::SelectEarth) ]
                            + SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
                            [ SNew(STextBlock).ColorAndOpacity(BlackText).Text(FText::FromString(TEXT("EARTH  (tap = select; green aura confirms it)"))) ]
                        ]

                        + SVerticalBox::Slot().AutoHeight().Padding(0, 7, 0, 3)
                        [ Section(TEXT("SHAPE")) ]
                        + SVerticalBox::Slot().AutoHeight().Padding(0, 4, 0, 4)
                        [
                            SNew(SHorizontalBox)
                            + SHorizontalBox::Slot().AutoWidth().Padding(0,0,14,0)[MakeShapeControl(ESpellLiveAction::SelectSphere, ESpellShape::Sphere, TEXT("Sphere"))]
                            + SHorizontalBox::Slot().AutoWidth().Padding(0,0,14,0)[MakeShapeControl(ESpellLiveAction::SelectCube, ESpellShape::Cube, TEXT("Cube"))]
                            + SHorizontalBox::Slot().AutoWidth()[MakeShapeControl(ESpellLiveAction::SelectCone, ESpellShape::Cone, TEXT("Cone"))]
                        ]

                        + SVerticalBox::Slot().AutoHeight().Padding(0, 7, 0, 3)
                        [ Section(TEXT("ORIENTATION")) ]
                        + SVerticalBox::Slot().AutoHeight().Padding(0, 4, 0, 4)
                        [
                            SNew(SHorizontalBox)
                            + SHorizontalBox::Slot().AutoWidth().Padding(0,0,14,0)
                            [ MakeOrientationControl(ESpellOrientationAxis::Forward, TEXT("Forward / X")) ]
                            + SHorizontalBox::Slot().AutoWidth().Padding(0,0,14,0)
                            [ MakeOrientationControl(ESpellOrientationAxis::Right, TEXT("Right / Y")) ]
                            + SHorizontalBox::Slot().AutoWidth()
                            [ MakeOrientationControl(ESpellOrientationAxis::Up, TEXT("Up / Z")) ]
                        ]

                        + SVerticalBox::Slot().AutoHeight().Padding(0, 7, 0, 3)
                        [ Section(TEXT("MULTIPLE OBJECTS")) ]

                        + SVerticalBox::Slot().AutoHeight().Padding(0,3)
                        [
                            SNew(STextBlock)
                            .ColorAndOpacity(BlackText)
                            .Text_Lambda([this]()
                            {
                                return FText::FromString(FString::Printf(
                                    TEXT("Amount: %d   (1 = single object; pattern activates above 1)"),
                                    ReadSpell().Pattern.Amount));
                            })
                        ]
                        + SVerticalBox::Slot().AutoHeight().Padding(0,0,0,9)
                        [
                            SNew(SSlider)
                            .IsFocusable(false)
                            .Value_Lambda([this]()
                            {
                                return static_cast<float>(ReadSpell().Pattern.Amount - SpellPatternRanges::MinAmount)
                                    / static_cast<float>(SpellPatternRanges::MaxAmount - SpellPatternRanges::MinAmount);
                            })
                            .OnValueChanged_Lambda([this](float V)
                            {
                                FSpellDefinition Spell = ReadSpell();
                                Spell.Pattern.Amount = FMath::Clamp(
                                    FMath::RoundToInt(FMath::Lerp(
                                        static_cast<float>(SpellPatternRanges::MinAmount),
                                        static_cast<float>(SpellPatternRanges::MaxAmount),
                                        V)),
                                    SpellPatternRanges::MinAmount,
                                    SpellPatternRanges::MaxAmount);
                                WriteSpell(Spell);
                            })
                        ]

                        + SVerticalBox::Slot().AutoHeight().Padding(0,3)
                        [
                            SNew(SBox)
                            .Visibility_Lambda([this]()
                            {
                                return ReadSpell().Pattern.Amount > 1
                                    ? EVisibility::Visible
                                    : EVisibility::Collapsed;
                            })
                            [
                                SNew(SHorizontalBox)
                                + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0,0,12,0)
                                [
                                    SNew(STextBlock)
                                    .ColorAndOpacity(BlackText)
                                    .Text(FText::FromString(TEXT("Arrangement:")))
                                ]
                                + SHorizontalBox::Slot().AutoWidth().Padding(0,0,10,0)
                                [ MakeArrangementControl(ESpellArrangement::Line, TEXT("Line")) ]
                                + SHorizontalBox::Slot().AutoWidth()
                                [ MakeArrangementControl(ESpellArrangement::Circle, TEXT("Circle")) ]
                            ]
                        ]

                        + SVerticalBox::Slot().AutoHeight().Padding(0,3)
                        [
                            SNew(SBox)
                            .Visibility_Lambda([this]()
                            {
                                const FSpellDefinition Spell = ReadSpell();
                                return Spell.Pattern.Amount > 1 && Spell.Pattern.Arrangement == ESpellArrangement::Line
                                    ? EVisibility::Visible : EVisibility::Collapsed;
                            })
                            [
                                SNew(SHorizontalBox)
                                + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0,0,12,0)
                                [
                                    SNew(STextBlock)
                                    .ColorAndOpacity(BlackText)
                                    .Text(FText::FromString(TEXT("Line axis:")))
                                ]
                                + SHorizontalBox::Slot().AutoWidth().Padding(0,0,10,0)
                                [ MakePatternAxisControl(ESpellPatternAxis::Forward, TEXT("Forward")) ]
                                + SHorizontalBox::Slot().AutoWidth().Padding(0,0,10,0)
                                [ MakePatternAxisControl(ESpellPatternAxis::Right, TEXT("Right")) ]
                                + SHorizontalBox::Slot().AutoWidth()
                                [ MakePatternAxisControl(ESpellPatternAxis::Up, TEXT("Up")) ]
                            ]
                        ]

                        + SVerticalBox::Slot().AutoHeight().Padding(0,3)
                        [
                            SNew(SBox)
                            .Visibility_Lambda([this]()
                            {
                                const FSpellDefinition Spell = ReadSpell();
                                return Spell.Pattern.Amount > 1 && Spell.Pattern.Arrangement == ESpellArrangement::Line
                                    ? EVisibility::Visible : EVisibility::Collapsed;
                            })
                            [
                                SNew(STextBlock)
                                .ColorAndOpacity(BlackText)
                                .Text_Lambda([this]()
                                {
                                    return FText::FromString(FString::Printf(
                                        TEXT("Spacing: %.0f cm"),
                                        ReadSpell().Pattern.SpacingCm));
                                })
                            ]
                        ]
                        + SVerticalBox::Slot().AutoHeight().Padding(0,0,0,9)
                        [
                            SNew(SBox)
                            .Visibility_Lambda([this]()
                            {
                                const FSpellDefinition Spell = ReadSpell();
                                return Spell.Pattern.Amount > 1 && Spell.Pattern.Arrangement == ESpellArrangement::Line
                                    ? EVisibility::Visible : EVisibility::Collapsed;
                            })
                            [
                                SNew(SSlider)
                                .IsFocusable(false)
                                .Value_Lambda([this]()
                                {
                                    return SpellParameterRanges::Normalize(
                                        ReadSpell().Pattern.SpacingCm,
                                        SpellPatternRanges::MinSpacingCm,
                                        SpellPatternRanges::MaxSpacingCm);
                                })
                                .OnValueChanged_Lambda([this](float V)
                                {
                                    FSpellDefinition Spell = ReadSpell();
                                    Spell.Pattern.SpacingCm = SpellParameterRanges::Denormalize(
                                        V,
                                        SpellPatternRanges::MinSpacingCm,
                                        SpellPatternRanges::MaxSpacingCm);
                                    WriteSpell(Spell);
                                })
                            ]
                        ]

                        + SVerticalBox::Slot().AutoHeight().Padding(0,3)
                        [
                            SNew(SBox)
                            .Visibility_Lambda([this]()
                            {
                                const FSpellDefinition Spell = ReadSpell();
                                return Spell.Pattern.Amount > 1 && Spell.Pattern.Arrangement == ESpellArrangement::Circle
                                    ? EVisibility::Visible : EVisibility::Collapsed;
                            })
                            [
                                SNew(STextBlock)
                                .ColorAndOpacity(BlackText)
                                .Text_Lambda([this]()
                                {
                                    return FText::FromString(FString::Printf(
                                        TEXT("Circle radius: %.0f cm"),
                                        ReadSpell().Pattern.CircleRadiusCm));
                                })
                            ]
                        ]
                        + SVerticalBox::Slot().AutoHeight().Padding(0,0,0,9)
                        [
                            SNew(SBox)
                            .Visibility_Lambda([this]()
                            {
                                const FSpellDefinition Spell = ReadSpell();
                                return Spell.Pattern.Amount > 1 && Spell.Pattern.Arrangement == ESpellArrangement::Circle
                                    ? EVisibility::Visible : EVisibility::Collapsed;
                            })
                            [
                                SNew(SSlider)
                                .IsFocusable(false)
                                .Value_Lambda([this]()
                                {
                                    return SpellParameterRanges::Normalize(
                                        ReadSpell().Pattern.CircleRadiusCm,
                                        SpellPatternRanges::MinCircleRadiusCm,
                                        SpellPatternRanges::MaxCircleRadiusCm);
                                })
                                .OnValueChanged_Lambda([this](float V)
                                {
                                    FSpellDefinition Spell = ReadSpell();
                                    Spell.Pattern.CircleRadiusCm = SpellParameterRanges::Denormalize(
                                        V,
                                        SpellPatternRanges::MinCircleRadiusCm,
                                        SpellPatternRanges::MaxCircleRadiusCm);
                                    WriteSpell(Spell);
                                })
                            ]
                        ]

                        + SVerticalBox::Slot().AutoHeight().Padding(0,7,0,3)
                        [
                            SNew(SBox)
                            .Visibility_Lambda([this]()
                            {
                                return ReadSpell().Pattern.Amount > 1
                                    ? EVisibility::Visible
                                    : EVisibility::Collapsed;
                            })
                            [
                                Section(TEXT("INSTANCE ORIENTATION"))
                            ]
                        ]

                        + SVerticalBox::Slot().AutoHeight().Padding(0,4,0,8)
                        [
                            SNew(SBox)
                            .Visibility_Lambda([this]()
                            {
                                return ReadSpell().Pattern.Amount > 1
                                    ? EVisibility::Visible
                                    : EVisibility::Collapsed;
                            })
                            [
                                SNew(SHorizontalBox)
                                + SHorizontalBox::Slot().AutoWidth().Padding(0,0,10,0)
                                [ MakePatternOrientationControl(ESpellPatternOrientation::Shared, TEXT("Shared")) ]
                                + SHorizontalBox::Slot().AutoWidth().Padding(0,0,10,0)
                                [ MakePatternOrientationControl(ESpellPatternOrientation::Outward, TEXT("Outward")) ]
                                + SHorizontalBox::Slot().AutoWidth().Padding(0,0,10,0)
                                [ MakePatternOrientationControl(ESpellPatternOrientation::Inward, TEXT("Inward")) ]
                                + SHorizontalBox::Slot().AutoWidth()
                                [ MakePatternOrientationControl(ESpellPatternOrientation::Tangent, TEXT("Tangent")) ]
                            ]
                        ]

                        + SVerticalBox::Slot().AutoHeight().Padding(0, 7, 0, 3)
                        [ Section(TEXT("SIZE / SHAPE DIMENSIONS")) ]
                        + SVerticalBox::Slot().AutoHeight()[MakeDimension(ESpellShape::Sphere, 0, ESpellLiveAction::SphereRadius)]
                        + SVerticalBox::Slot().AutoHeight()[MakeDimension(ESpellShape::Cube, 0, ESpellLiveAction::CubeX)]
                        + SVerticalBox::Slot().AutoHeight()[MakeDimension(ESpellShape::Cube, 1, ESpellLiveAction::CubeY)]
                        + SVerticalBox::Slot().AutoHeight()[MakeDimension(ESpellShape::Cube, 2, ESpellLiveAction::CubeZ)]
                        + SVerticalBox::Slot().AutoHeight()[MakeDimension(ESpellShape::Cone, 0, ESpellLiveAction::ConeRadius)]
                        + SVerticalBox::Slot().AutoHeight()[MakeDimension(ESpellShape::Cone, 1, ESpellLiveAction::ConeHeight)]

                        + SVerticalBox::Slot().AutoHeight().Padding(0, 7, 0, 3)
                        [ Section(TEXT("MOTION / MATERIAL")) ]

                        + SVerticalBox::Slot().AutoHeight().Padding(0, 4, 0, 3)
                        [
                            SNew(STextBlock)
                            .ColorAndOpacity(BlackText)
                            .Text(FText::FromString(TEXT("DIRECTION / TRAVEL   (independent from shape orientation)")))
                        ]

                        + SVerticalBox::Slot().AutoHeight().Padding(0, 2, 0, 3)
                        [
                            SNew(SHorizontalBox)
                            + SHorizontalBox::Slot().AutoWidth().Padding(0,0,8,0)
                            [ MakeMotionDirectionControl(ESpellMotionDirection::Forward, TEXT("Forward")) ]
                            + SHorizontalBox::Slot().AutoWidth().Padding(0,0,8,0)
                            [ MakeMotionDirectionControl(ESpellMotionDirection::Backward, TEXT("Backward")) ]
                            + SHorizontalBox::Slot().AutoWidth().Padding(0,0,8,0)
                            [ MakeMotionDirectionControl(ESpellMotionDirection::Up, TEXT("Up")) ]
                            + SHorizontalBox::Slot().AutoWidth()
                            [ MakeMotionDirectionControl(ESpellMotionDirection::Down, TEXT("Down")) ]
                        ]

                        + SVerticalBox::Slot().AutoHeight().Padding(0, 2, 0, 9)
                        [
                            SNew(SHorizontalBox)
                            + SHorizontalBox::Slot().AutoWidth().Padding(0,0,8,0)
                            [ MakeMotionDirectionControl(ESpellMotionDirection::Outward, TEXT("Outward")) ]
                            + SHorizontalBox::Slot().AutoWidth().Padding(0,0,8,0)
                            [ MakeMotionDirectionControl(ESpellMotionDirection::Inward, TEXT("Inward")) ]
                            + SHorizontalBox::Slot().AutoWidth()
                            [ MakeMotionDirectionControl(ESpellMotionDirection::Tangent, TEXT("Tangent")) ]
                        ]

                        + SVerticalBox::Slot().AutoHeight().Padding(0,3)
                        [
                            SNew(SHorizontalBox)
                            + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0,0,8,0)[MakeBindSelector(ESpellLiveAction::Speed)]
                            + SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
                            [ SNew(STextBlock).ColorAndOpacity(BlackText).Text_Lambda([this](){ return FText::FromString(FString::Printf(TEXT("Speed: %.1f m/s   (0 = create; gravity still applies)"), ReadSpell().SpeedMps)); }) ]
                        ]
                        + SVerticalBox::Slot().AutoHeight().Padding(100,0,0,9)
                        [ SNew(SSlider).IsFocusable(false).Value_Lambda([this](){ return GetSpeedSlider(); }).OnValueChanged_Lambda([this](float V){ SetSpeedSlider(V); }) ]

                        + SVerticalBox::Slot().AutoHeight().Padding(0,3)
                        [
                            SNew(SHorizontalBox)
                            + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0,0,8,0)[MakeBindSelector(ESpellLiveAction::Density)]
                            + SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
                            [ SNew(STextBlock).ColorAndOpacity(BlackText).Text_Lambda([this](){ return FText::FromString(FString::Printf(TEXT("Density: %.0f kg/m3   |   derived mass: %.1f kg"), ReadSpell().Material.DensityKgPerM3, FSpellShapeMath::CalculateMassKg(ReadSpell()))); }) ]
                        ]
                        + SVerticalBox::Slot().AutoHeight().Padding(100,0,0,9)
                        [ SNew(SSlider).IsFocusable(false).Value_Lambda([this](){ return GetDensitySlider(); }).OnValueChanged_Lambda([this](float V){ SetDensitySlider(V); }) ]

                        + SVerticalBox::Slot().AutoHeight().Padding(0,3)
                        [
                            SNew(SHorizontalBox)
                            + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0,0,8,0)[MakeBindSelector(ESpellLiveAction::Hardness)]
                            + SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
                            [ SNew(STextBlock).ColorAndOpacity(BlackText).Text_Lambda([this](){ return FText::FromString(FString::Printf(TEXT("Hardness: %.2f"), ReadSpell().Material.Hardness)); }) ]
                        ]
                        + SVerticalBox::Slot().AutoHeight().Padding(100,0,0,9)
                        [ SNew(SSlider).IsFocusable(false).Value_Lambda([this](){ return GetHardnessSlider(); }).OnValueChanged_Lambda([this](float V){ SetHardnessSlider(V); }) ]

                        + SVerticalBox::Slot().AutoHeight().Padding(0,3)
                        [
                            SNew(SHorizontalBox)
                            + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0,0,8,0)[MakeBindSelector(ESpellLiveAction::Toughness)]
                            + SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
                            [ SNew(STextBlock).ColorAndOpacity(BlackText).Text_Lambda([this](){ return FText::FromString(FString::Printf(TEXT("Toughness: %.2f"), ReadSpell().Material.Toughness)); }) ]
                        ]
                        + SVerticalBox::Slot().AutoHeight().Padding(100,0,0,9)
                        [ SNew(SSlider).IsFocusable(false).Value_Lambda([this](){ return GetToughnessSlider(); }).OnValueChanged_Lambda([this](float V){ SetToughnessSlider(V); }) ]

                        + SVerticalBox::Slot().AutoHeight().Padding(0,3)
                        [
                            SNew(SHorizontalBox)
                            + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0,0,8,0)[MakeBindSelector(ESpellLiveAction::Elasticity)]
                            + SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
                            [ SNew(STextBlock).ColorAndOpacity(BlackText).Text_Lambda([this](){ return FText::FromString(FString::Printf(TEXT("Elasticity: %.2f"), ReadSpell().Material.Restitution)); }) ]
                        ]
                        + SVerticalBox::Slot().AutoHeight().Padding(100,0,0,9)
                        [ SNew(SSlider).IsFocusable(false).Value_Lambda([this](){ return GetElasticitySlider(); }).OnValueChanged_Lambda([this](float V){ SetElasticitySlider(V); }) ]

                        + SVerticalBox::Slot().AutoHeight().Padding(0,7,0,3)
                        [ Section(TEXT("SPATIAL")) ]
                        + SVerticalBox::Slot().AutoHeight().Padding(0,3)
                        [
                            SNew(SHorizontalBox)
                            + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0,0,8,0)[MakeBindSelector(ESpellLiveAction::Distance)]
                            + SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
                            [ SNew(STextBlock).ColorAndOpacity(BlackText).Text_Lambda([this](){ return FText::FromString(FString::Printf(TEXT("Distance: %.1f m   (additional distance from safe point in front of character)"), ReadSpell().DistanceM)); }) ]
                        ]
                        + SVerticalBox::Slot().AutoHeight().Padding(100,0,0,9)
                        [ SNew(SSlider).IsFocusable(false).Value_Lambda([this](){ return GetDistanceSlider(); }).OnValueChanged_Lambda([this](float V){ SetDistanceSlider(V); }) ]

                        + SVerticalBox::Slot().AutoHeight().Padding(0,10,0,0)
                        [ SNew(STextBlock).ColorAndOpacity(BlackText).Text(FText::FromString(TEXT("DEFAULT LIVE TEST: Q Earth | A primary shape dimension | S Speed | D Density | SPACE casts only after construction starts."))) ]
                    ]
                ]
            ]
        ];

    GEngine->GameViewport->AddViewportWidgetContent(EditorWidget.ToSharedRef(), 5000);

    PreviewFrameWidget = SNew(SBox)
        .Visibility_Lambda([this]()
        {
            // Preview is presentation-only. Shared interactive controls now live
            // in the InnerRealm shell above every page, so the preview can be
            // completely transparent to mouse hit testing.
            return (ActivePage == EInnerRealmPage::Spell ||
                    ActivePage == EInnerRealmPage::Canvas)
                ? EVisibility::HitTestInvisible
                : EVisibility::Collapsed;
        })
        .HAlign(HAlign_Right)
        .VAlign(VAlign_Center)
        [
            SNew(SBox)
            .WidthOverride(520.0f)
            .HeightOverride(760.0f)
            [
                SNew(SBorder)
                .Padding(FMargin(16.0f))
                .BorderBackgroundColor(FLinearColor(0.05f, 0.08f, 0.05f, 0.12f))
                [
                    SNew(SVerticalBox)

                    + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 0, 0, 8)
                    [
                        SNew(STextBlock)
                        .ColorAndOpacity(FSlateColor(FLinearColor(0.86f, 0.96f, 0.87f, 1.0f)))
                        .Text(FText::FromString(TEXT("SPELL WORKBENCH / 3D PREVIEW")))
                    ]

                    + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
                    [
                        SNew(STextBlock)
                        .ColorAndOpacity(FSlateColor(FLinearColor(0.72f, 0.88f, 0.74f, 1.0f)))
                        .Text_Lambda([this]() { return GetCurrentSpellSummary(); })
                    ]

                    + SVerticalBox::Slot().FillHeight(1.0f)
                    [
                        SNew(SBox)
                    ]

                    + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 8, 0, 0)
                    [
                        SNew(STextBlock)
                        .ColorAndOpacity(FSlateColor(FLinearColor(0.78f, 0.90f, 0.80f, 1.0f)))
                        .Text(FText::FromString(TEXT("Reference mannequin shows spell scale, distance, density grid and speed rings.")))
                    ]
                ]
            ]
        ];

    GEngine->GameViewport->AddViewportWidgetContent(PreviewFrameWidget.ToSharedRef(), 4999);


    TSharedRef<SVerticalBox> CodexList = SNew(SVerticalBox);
    if (UWorldCodexSubsystem* Codex = GetWorldCodex())
    {
        const ECodexCategory CategoryOrder[] =
        {
            ECodexCategory::Element,
            ECodexCategory::Shape,
            ECodexCategory::ShapeParameter,
            ECodexCategory::MaterialProperty,
            ECodexCategory::Spatial,
            ECodexCategory::Pattern,
            ECodexCategory::Motion,
            ECodexCategory::Action,
            ECodexCategory::WorldObject,
            ECodexCategory::WorldState,
            ECodexCategory::Event,
            ECodexCategory::Logic,
            ECodexCategory::Value,
            ECodexCategory::PhysicsConcept
        };

        for (const ECodexCategory Category : CategoryOrder)
        {
            const TArray<const FCodexEntry*> Entries = Codex->GetEntriesByCategory(Category);
            if (Entries.Num() == 0) continue;

            CodexList->AddSlot().AutoHeight().Padding(0, 9, 0, 4)
            [
                SNew(SBorder)
                .Padding(FMargin(7.0f, 5.0f))
                .BorderBackgroundColor(FLinearColor(0.78f, 0.76f, 0.70f, 0.96f))
                [
                    SNew(STextBlock)
                    .ColorAndOpacity(BlackText)
                    .Text(CodexCategoryToText(Category))
                ]
            ];

            for (const FCodexEntry* Entry : Entries)
            {
                if (!Entry) continue;
                const FName EntryId = Entry->ConceptId;

                CodexList->AddSlot().AutoHeight().Padding(0, 1)
                [
                    SNew(SButton)
                    .IsFocusable(false)
                    .ContentPadding(FMargin(8.0f, 5.0f))
                    .OnClicked_Lambda([this, EntryId]()
                    {
                        SelectedCodexConcept = EntryId;
                        return FReply::Handled();
                    })
                    [
                        SNew(STextBlock)
                        .ColorAndOpacity(BlackText)
                        .Text_Lambda([this, EntryId]()
                        {
                            const UWorldCodexSubsystem* CurrentCodex = GetWorldCodex();
                            const FCodexEntry* CurrentEntry = CurrentCodex
                                ? CurrentCodex->FindEntry(EntryId)
                                : nullptr;
                            if (!CurrentEntry) return FText::FromString(EntryId.ToString());

                            if (SelectedCodexConcept == EntryId)
                            {
                                return FText::FromString(FString::Printf(
                                    TEXT("[ %s ]  %s"),
                                    *CurrentEntry->Sign.Glyph,
                                    *CurrentEntry->DisplayName.ToString()));
                            }

                            return FText::FromString(FString::Printf(
                                TEXT("  %s    %s"),
                                *CurrentEntry->Sign.Glyph,
                                *CurrentEntry->DisplayName.ToString()));
                        })
                    ]
                ];
            }
        }
    }

    CodexWidget = SNew(SBox)
        .Visibility_Lambda([this]() { return ActivePage == EInnerRealmPage::Codex ? EVisibility::Visible : EVisibility::Collapsed; })
        .HAlign(HAlign_Center)
        .VAlign(VAlign_Center)
        [
            SNew(SBox)
            .WidthOverride(1340.0f)
            .HeightOverride(760.0f)
            [
                SNew(SBorder)
                .Padding(FMargin(20.0f))
                .BorderBackgroundColor(FLinearColor(0.92f, 0.90f, 0.84f, 0.98f))
                [
                    SNew(SHorizontalBox)
                    + SHorizontalBox::Slot().FillWidth(0.34f).Padding(0, 0, 18, 0)
                    [
                        SNew(SVerticalBox)
                        + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 8)
                        [
                            SNew(STextBlock)
                            .ColorAndOpacity(BlackText)
                            .Text(FText::FromString(TEXT("WORLD CODEX / CANONICAL VOCABULARY")))
                        ]
                        + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 8)
                        [
                            SNew(STextBlock)
                            .ColorAndOpacity(BlackText)
                            .AutoWrapText(true)
                            .Text(FText::FromString(TEXT("Everything meaningful in the world can receive a stable Concept ID and Sign. Implemented and future concepts live in the same dictionary.")))
                        ]
                        + SVerticalBox::Slot().FillHeight(1.0f)
                        [
                            SNew(SScrollBox)
                            + SScrollBox::Slot()
                            [
                                CodexList
                            ]
                        ]
                    ]
                    + SHorizontalBox::Slot().FillWidth(0.66f)
                    [
                        SNew(SBorder)
                        .Padding(FMargin(22.0f))
                        .BorderBackgroundColor(FLinearColor(0.82f, 0.81f, 0.75f, 0.70f))
                        [
                            SNew(SScrollBox)
                            + SScrollBox::Slot()
                            [
                                SNew(SVerticalBox)
                                + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 8)
                                [
                                    SNew(STextBlock)
                                    .ColorAndOpacity(BlackText)
                                    .Text_Lambda([this]() { return GetSelectedCodexTitle(); })
                                ]
                                + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 18)
                                [
                                    SNew(STextBlock)
                                    .ColorAndOpacity(BlackText)
                                    .Text_Lambda([this]() { return GetSelectedCodexSign(); })
                                ]
                                + SVerticalBox::Slot().AutoHeight()
                                [
                                    SNew(STextBlock)
                                    .ColorAndOpacity(BlackText)
                                    .AutoWrapText(true)
                                    .Text_Lambda([this]() { return GetSelectedCodexDetails(); })
                                ]
                            ]
                        ]
                    ]
                ]
            ]
        ];

    GEngine->GameViewport->AddViewportWidgetContent(CodexWidget.ToSharedRef(), 4998);

    RebuildCanvasWidget();

    FInnerRealmShellCallbacks ShellCallbacks;

    ShellCallbacks.GetActivePage = [this]()
    {
        return ActivePage;
    };

    ShellCallbacks.SetActivePage = [this](const EInnerRealmPage Page)
    {
        ActivePage = Page;
    };

    ShellCallbacks.SavePreparedSlot = [this](const int32 SlotIndex)
    {
        UWorld* World = GetWorld();
        USpellCreationSubsystem* Creation = GetSpellCreation();
        USpellLoadoutSubsystem* Loadout =
            World ? World->GetSubsystem<USpellLoadoutSubsystem>() : nullptr;

        if (Creation && Loadout)
        {
            Loadout->SaveSlot(
                SlotIndex,
                Creation->GetStoredGenericSpellDefinition());
            Loadout->EquipSlot(SlotIndex);
        }
    };

    ShellCallbacks.IsPreparedSlotOccupied = [this](const int32 SlotIndex)
    {
        UWorld* World = GetWorld();
        const USpellLoadoutSubsystem* Loadout =
            World ? World->GetSubsystem<USpellLoadoutSubsystem>() : nullptr;
        return Loadout && Loadout->IsSlotOccupied(SlotIndex);
    };

    ShellWidget = FInnerRealmShellUI::Build(ShellCallbacks);

    // Shared shell is always above page content. Canvas/Workbench/Codex can no
    // longer make the prepared-spell buttons unreachable through Z-order.
    GEngine->GameViewport->AddViewportWidgetContent(
        ShellWidget.ToSharedRef(),
        5003);
}

void UInnerRealmSubsystem::RemoveEditorWidget()
{
    if (EditorWidget.IsValid() && GEngine && GEngine->GameViewport)
    {
        GEngine->GameViewport->RemoveViewportWidgetContent(EditorWidget.ToSharedRef());
    }
    if (PreviewFrameWidget.IsValid() && GEngine && GEngine->GameViewport)
    {
        GEngine->GameViewport->RemoveViewportWidgetContent(PreviewFrameWidget.ToSharedRef());
    }
    if (CodexWidget.IsValid() && GEngine && GEngine->GameViewport)
    {
        GEngine->GameViewport->RemoveViewportWidgetContent(CodexWidget.ToSharedRef());
    }
    if (CanvasWidget.IsValid() && GEngine && GEngine->GameViewport)
    {
        GEngine->GameViewport->RemoveViewportWidgetContent(CanvasWidget.ToSharedRef());
    }
    if (ShellWidget.IsValid() && GEngine && GEngine->GameViewport)
    {
        GEngine->GameViewport->RemoveViewportWidgetContent(ShellWidget.ToSharedRef());
    }

    ShellWidget.Reset();
    EditorWidget.Reset();
    PreviewFrameWidget.Reset();
    CodexWidget.Reset();
    CanvasWidget.Reset();
}
