#include "Tools/Workflow/DevKitWorkbenchUI.h"

#include "Styling/AppStyle.h"
#include "Widgets/Colors/SColorBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

TSharedRef<SExpandableArea> DevKitWorkbenchUI::MakeSection(const FText& Title,
    TSharedRef<SWidget> Body, bool bInitiallyCollapsed, const FText& Hint,
    FSlateColor Accent, FOnBooleanValueChanged OnExpansionChanged)
{
    return SNew(SExpandableArea)
        .Style(&FAppStyle::Get().GetWidgetStyle<FExpandableAreaStyle>("ExpandableArea"))
        .BorderImage(FAppStyle::GetBrush("Brushes.Panel"))
        .BodyBorderImage(FAppStyle::GetBrush("Brushes.Panel"))
        .HeaderPadding(FMargin(10.f, 8.f)).Padding(FMargin(10.f, 4.f, 10.f, 10.f))
        .InitiallyCollapsed(bInitiallyCollapsed).AllowAnimatedTransition(false)
        .OnAreaExpansionChanged(MoveTemp(OnExpansionChanged))
        .HeaderContent()
        [SNew(STextBlock).Text(Title).ToolTipText(Hint)
            .Font(FAppStyle::GetFontStyle("DetailsView.CategoryFontStyle"))
            .ColorAndOpacity(Accent)]
        .BodyContent()
        [SNew(SVerticalBox)
            + SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 8.f)
            [SNew(STextBlock).Text(Hint).AutoWrapText(true)
                .Visibility(Hint.IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible)
                .ColorAndOpacity(FSlateColor::UseSubduedForeground())]
            + SVerticalBox::Slot().AutoHeight()[Body]];
}

void SDevKitWorkbenchColorPicker::Construct(const FArguments& InArgs)
{
    ColorAttribute = InArgs._Color;
    ColorChanged = InArgs._OnColorChanged;
    PickerOpenChanged = InArgs._OnPickerOpenChanged;
    bUseAlpha = InArgs._UseAlpha;
    bClampValue = InArgs._ClampValue;
    ChildSlot
    [SNew(SButton).ContentPadding(3.f)
        .ToolTipText(FText::FromString(TEXT("打开 UE 色板；修改实时预览，取消恢复打开前的颜色。")))
        .OnClicked(this, &SDevKitWorkbenchColorPicker::OpenPicker)
        [SNew(SColorBlock).Color_Lambda([this]() { return ColorAttribute.Get(FLinearColor::White); })
            .AlphaDisplayMode(bUseAlpha ? EColorBlockAlphaDisplayMode::Separate : EColorBlockAlphaDisplayMode::Ignore)
            .ShowBackgroundForAlpha(bUseAlpha).UseSRGB(true).Size(FVector2D(120.f, 24.f))]];
}

FLinearColor SDevKitWorkbenchColorPicker::SanitizeColor(FLinearColor Color, FLinearColor Fallback) const
{
    const auto Channel = [this](float Value, float Original)
    {
        if (!FMath::IsFinite(Value)) Value = FMath::IsFinite(Original) ? Original : 1.f;
        return bClampValue ? FMath::Clamp(Value, 0.f, 1.f) : Value;
    };
    Color.R = Channel(Color.R, Fallback.R);
    Color.G = Channel(Color.G, Fallback.G);
    Color.B = Channel(Color.B, Fallback.B);
    Color.A = bUseAlpha ? FMath::Clamp(FMath::IsFinite(Color.A) ? Color.A : Fallback.A, 0.f, 1.f)
        : Fallback.A;
    if (!FMath::IsFinite(Color.A)) Color.A = 1.f;
    return Color;
}

FColorPickerArgs SDevKitWorkbenchColorPicker::MakePickerArgs()
{
    // Preserve the complete original color, including an unused alpha and HDR channels.
    OpeningColor = ColorAttribute.Get(FLinearColor::White);
    for (int32 Channel = 0; Channel < 4; ++Channel)
        if (!FMath::IsFinite(OpeningColor.Component(Channel))) OpeningColor.Component(Channel) = 1.f;
    bPickerOpen = true;
    PickerOpenChanged.ExecuteIfBound(true);
    FColorPickerArgs Args;
    Args.ParentWidget = AsShared();
    Args.InitialColor = SanitizeColor(OpeningColor, OpeningColor);
    Args.bUseAlpha = bUseAlpha;
    Args.bClampValue = bClampValue;
    Args.bOnlyRefreshOnMouseUp = false;
    Args.bOnlyRefreshOnOk = false;
    Args.OnColorCommitted = FOnLinearColorValueChanged::CreateSP(this, &SDevKitWorkbenchColorPicker::PreviewColor);
    Args.OnColorPickerCancelled = FOnColorPickerCancelled::CreateSP(this, &SDevKitWorkbenchColorPicker::CancelColor);
    Args.OnColorPickerWindowClosed = FOnWindowClosed::CreateSP(this, &SDevKitWorkbenchColorPicker::PickerClosed);
    return Args;
}

FReply SDevKitWorkbenchColorPicker::OpenPicker()
{
    if (bPickerOpen || bOwnerShuttingDown) return FReply::Handled();
    if (!OpenColorPicker(MakePickerArgs()))
    {
        bPickerOpen = false;
        PickerOpenChanged.ExecuteIfBound(false);
    }
    else OwnedNativePicker = GetColorPicker();
    return FReply::Handled();
}

void SDevKitWorkbenchColorPicker::CancelForOwnerShutdown()
{
    if (bOwnerShuttingDown) return;
    bOwnerShuttingDown = true;

    // Do not leave a preview in the live settings when its editing owner goes away.
    // The owner calls us BEFORE member destruction; the helper's destructor must
    // never invoke a callback that captures an already destructing parent.
    if (bPickerOpen) CancelColor(OpeningColor);
    const bool bWasOpen = bPickerOpen;
    bPickerOpen = false;
    if (bWasOpen) PickerOpenChanged.ExecuteIfBound(false);
    ColorChanged.Unbind();
    PickerOpenChanged.Unbind();
    ColorAttribute = FLinearColor::White;

    // UE has a global native picker. A different tool may have replaced ours,
    // so an owner's shutdown must never destroy that other tool's picker.
    const TSharedPtr<SColorPicker> NativePicker = OwnedNativePicker.Pin();
    OwnedNativePicker.Reset();
    if (NativePicker.IsValid() && NativePicker == GetColorPicker()) DestroyColorPicker();
}

void SDevKitWorkbenchColorPicker::PreviewColor(FLinearColor Color)
{
    if (bPickerOpen) ColorChanged.ExecuteIfBound(SanitizeColor(Color, OpeningColor));
}

void SDevKitWorkbenchColorPicker::CancelColor(FLinearColor)
{
    if (bPickerOpen) ColorChanged.ExecuteIfBound(OpeningColor);
}

void SDevKitWorkbenchColorPicker::PickerClosed(const TSharedRef<SWindow>&)
{
    if (!bPickerOpen) return;
    bPickerOpen = false;
    OwnedNativePicker.Reset();
    PickerOpenChanged.ExecuteIfBound(false);
}
