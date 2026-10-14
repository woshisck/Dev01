#pragma once

#include "CoreMinimal.h"
#include "Framework/SlateDelegates.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Layout/SExpandableArea.h"
#include "Widgets/Colors/SColorPicker.h"

/** Presentation-only helpers. They do not save configuration, transact assets or apply rendering settings. */
namespace DevKitWorkbenchUI
{
    TSharedRef<SExpandableArea> MakeSection(const FText& Title, TSharedRef<SWidget> Body,
        bool bInitiallyCollapsed = false, const FText& Hint = FText::GetEmpty(),
        FSlateColor Accent = FSlateColor::UseForeground(),
        FOnBooleanValueChanged OnExpansionChanged = FOnBooleanValueChanged());

    inline TSharedRef<SExpandableArea> MakeSection(const TCHAR* Title, TSharedRef<SWidget> Body,
        bool bInitiallyCollapsed = false, const TCHAR* Hint = TEXT(""),
        FSlateColor Accent = FSlateColor::UseForeground(),
        FOnBooleanValueChanged OnExpansionChanged = FOnBooleanValueChanged())
    {
        return MakeSection(FText::FromString(Title), Body, bInitiallyCollapsed,
            FText::FromString(Hint), Accent, MoveTemp(OnExpansionChanged));
    }
}

/** UE's native color picker. Values remain linear; preview and cancel use the caller's existing update path. */
class SDevKitWorkbenchColorPicker : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SDevKitWorkbenchColorPicker)
        : _Color(FLinearColor::White), _UseAlpha(false), _ClampValue(false) {}
        SLATE_ATTRIBUTE(FLinearColor, Color)
        SLATE_ARGUMENT(bool, UseAlpha)
        // True for a 0..1 tint; leave false for existing HDR color/intensity parameters.
        SLATE_ARGUMENT(bool, ClampValue)
        SLATE_EVENT(FOnLinearColorValueChanged, OnColorChanged)
        SLATE_EVENT(FOnBooleanValueChanged, OnPickerOpenChanged)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);
    bool IsPickerOpen() const { return bPickerOpen; }
    /** Call from the owner's destructor body, before its live update state is destroyed. */
    void CancelForOwnerShutdown();

private:
    friend class FDevKitWorkbenchColorPickerTest;
    friend class FDevKitShadowTransitionWidgetTest;
    friend class FDevKitShadowTransitionOwnerShutdownTest;
    FReply OpenPicker();
    FColorPickerArgs MakePickerArgs();
    void PreviewColor(FLinearColor Color);
    void CancelColor(FLinearColor Color);
    void PickerClosed(const TSharedRef<SWindow>& Window);
    FLinearColor SanitizeColor(FLinearColor Color, FLinearColor Fallback) const;

    TAttribute<FLinearColor> ColorAttribute;
    FOnLinearColorValueChanged ColorChanged;
    FOnBooleanValueChanged PickerOpenChanged;
    FLinearColor OpeningColor = FLinearColor::White;
    bool bUseAlpha = false;
    bool bClampValue = false;
    bool bPickerOpen = false;
    bool bOwnerShuttingDown = false;
    TWeakPtr<SColorPicker> OwnedNativePicker;
};
