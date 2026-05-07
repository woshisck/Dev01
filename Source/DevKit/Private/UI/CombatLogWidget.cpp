#include "UI/CombatLogWidget.h"
#include "Components/Button.h"
#include "Components/Border.h"
#include "Components/CheckBox.h"
#include "Components/ComboBoxString.h"
#include "Components/HorizontalBox.h"
#include "Components/PanelWidget.h"
#include "Components/Slider.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/ScrollBox.h"
#include "Components/WrapBox.h"
#include "Components/WrapBoxSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Blueprint/WidgetTree.h"

namespace
{
constexpr float CombatLogOuterPadding = 12.f;
constexpr float CombatLogTopPadding = 10.f;
constexpr float CombatLogPanelGap = 8.f;

FLinearColor GetFilterTint(ECombatLogFilter Filter)
{
	switch (Filter)
	{
	case ECombatLogFilter::Normal:   return FLinearColor(0.72f, 0.76f, 0.76f, 1.f);
	case ECombatLogFilter::Crit:     return FLinearColor(1.f, 0.78f, 0.18f, 1.f);
	case ECombatLogFilter::Rune:     return FLinearColor(0.78f, 0.48f, 1.f, 1.f);
	case ECombatLogFilter::Bleed:    return FLinearColor(1.f, 0.24f, 0.24f, 1.f);
	case ECombatLogFilter::Card:     return FLinearColor(0.42f, 0.7f, 1.f, 1.f);
	case ECombatLogFilter::Finisher: return FLinearColor(1.f, 0.84f, 0.2f, 1.f);
	case ECombatLogFilter::Link:     return FLinearColor(1.f, 0.55f, 0.16f, 1.f);
	case ECombatLogFilter::Shuffle:  return FLinearColor(0.52f, 0.72f, 1.f, 1.f);
	default:                         return FLinearColor(0.9f, 0.96f, 0.96f, 1.f);
	}
}
}

// ── UCombatFilterProxy ────────────────────────────────────────────────────
void UCombatFilterProxy::OnClicked()
{
	if (Owner.IsValid())
		Owner->SetFilter(Filter);
}

// ── UCombatLogWidget ──────────────────────────────────────────────────────
int32 UCombatLogWidget::GetEffectiveLogFontSize() const
{
	const int32 SafeMin = FMath::Max(8, MinLogFontSize);
	const int32 SafeMax = FMath::Max(SafeMin, MaxLogFontSize);
	return FMath::Clamp(LogFontSize, SafeMin, SafeMax);
}

int32 UCombatLogWidget::GetEffectiveFilterFontSize() const
{
	return FMath::Clamp(FilterFontSize, 9, 14);
}

int32 UCombatLogWidget::GetEffectiveSummaryFontSize() const
{
	return FMath::Clamp(SummaryFontSize, 9, 13);
}

void UCombatLogWidget::ConfigureWidgetLayout()
{
	UWidget* Root = WidgetTree ? WidgetTree->RootWidget : nullptr;
	UCanvasPanel* Canvas = Cast<UCanvasPanel>(Root);
	if (!Canvas)
	{
		ApplySummaryTextStyle();
		return;
	}

	if (!WidgetTree->FindWidget(FName(TEXT("CombatLogRuntimeBackdrop"))))
	{
		UBorder* Backdrop = NewObject<UBorder>(this, TEXT("CombatLogRuntimeBackdrop"));
		Backdrop->SetBrushColor(PanelBackgroundColor);
		Backdrop->SetVisibility(ESlateVisibility::HitTestInvisible);
		if (UCanvasPanelSlot* BackdropSlot = Canvas->AddChildToCanvas(Backdrop))
		{
			BackdropSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
			BackdropSlot->SetPosition(FVector2D::ZeroVector);
			BackdropSlot->SetSize(FVector2D::ZeroVector);
			BackdropSlot->SetAutoSize(false);
			BackdropSlot->SetZOrder(-10);
		}
	}

	const float SafeControlHeight = FMath::Clamp(ControlBarHeight, 76.f, 140.f);
	const float SafeSummaryHeight = FMath::Clamp(SummaryHeight, 84.f, 180.f);
	const float LogTop = CombatLogTopPadding + SafeControlHeight + CombatLogPanelGap;
	const float LogBottom = SafeSummaryHeight + CombatLogOuterPadding + CombatLogPanelGap;

	if (FilterButtonBox)
	{
		FilterButtonBox->SetInnerSlotPadding(FVector2D(6.f, 6.f));
		if (UCanvasPanelSlot* FilterCanvasSlot = Cast<UCanvasPanelSlot>(FilterButtonBox->Slot))
		{
			FilterCanvasSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 0.f));
			FilterCanvasSlot->SetPosition(FVector2D(CombatLogOuterPadding, CombatLogTopPadding));
			FilterCanvasSlot->SetSize(FVector2D(-CombatLogOuterPadding, SafeControlHeight));
			FilterCanvasSlot->SetAutoSize(false);
			FilterCanvasSlot->SetZOrder(20);
		}
	}

	if (LogScrollBox)
	{
		if (UCanvasPanelSlot* LogCanvasSlot = Cast<UCanvasPanelSlot>(LogScrollBox->Slot))
		{
			LogCanvasSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
			LogCanvasSlot->SetPosition(FVector2D(CombatLogOuterPadding, LogTop));
			LogCanvasSlot->SetSize(FVector2D(-CombatLogOuterPadding, -LogBottom));
			LogCanvasSlot->SetAutoSize(false);
			LogCanvasSlot->SetZOrder(10);
		}
	}

	if (SummaryText)
	{
		if (UCanvasPanelSlot* SummaryCanvasSlot = Cast<UCanvasPanelSlot>(SummaryText->Slot))
		{
			SummaryCanvasSlot->SetAnchors(FAnchors(0.f, 1.f, 1.f, 1.f));
			SummaryCanvasSlot->SetPosition(FVector2D(CombatLogOuterPadding, -SafeSummaryHeight - CombatLogOuterPadding));
			SummaryCanvasSlot->SetSize(FVector2D(-CombatLogOuterPadding, SafeSummaryHeight));
			SummaryCanvasSlot->SetAutoSize(false);
			SummaryCanvasSlot->SetZOrder(15);
		}
	}

	ApplySummaryTextStyle();
}

void UCombatLogWidget::ApplySummaryTextStyle()
{
	if (!SummaryText)
	{
		return;
	}

	SummaryText->SetColorAndOpacity(FSlateColor(SummaryTextColor));
	SummaryText->SetAutoWrapText(true);
	SummaryText->SetShadowOffset(FVector2D(1.f, 1.f));
	SummaryText->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.7f));

	FSlateFontInfo FontInfo = SummaryText->GetFont();
	FontInfo.Size = GetEffectiveSummaryFontSize();
	SummaryText->SetFont(FontInfo);
}

void UCombatLogWidget::NativeConstruct()
{
	Super::NativeConstruct();

	ActiveTimeWindowSeconds = DefaultTimeWindowSeconds;
	bDebugMode = bDefaultDebugMode;

	// FilterButtonBox 未在 Blueprint 里绑定时，自动插入到根 CanvasPanel
	if (!FilterButtonBox)
	{
		UWidget* Root = WidgetTree ? WidgetTree->RootWidget : nullptr;

		if (UCanvasPanel* Canvas = Cast<UCanvasPanel>(Root))
		{
			FilterButtonBox = NewObject<UWrapBox>(this, TEXT("FilterButtonBox"));
			if (UCanvasPanelSlot* CanvasSlot = Canvas->AddChildToCanvas(FilterButtonBox))
			{
				CanvasSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 0.f));
				CanvasSlot->SetPosition(FVector2D(0.f, 0.f));
				CanvasSlot->SetSize(FVector2D(0.f, 92.f));
				CanvasSlot->SetAutoSize(false);
			}
		}
		else if (LogScrollBox)
		{
			// 兜底：挂到 LogScrollBox 的父面板末尾
			if (UPanelWidget* Parent = Cast<UPanelWidget>(LogScrollBox->GetParent()))
			{
				FilterButtonBox = NewObject<UWrapBox>(this, TEXT("FilterButtonBox"));
				Parent->AddChild(FilterButtonBox);
			}
		}
	}

	ConfigureWidgetLayout();
	BuildFilterButtons();
	RefreshFilterOptions();
	RefreshTimeWindowText();
}

void UCombatLogWidget::BuildFilterButtons()
{
	if (!FilterButtonBox)
	{
		return;
	}

	FilterButtonBox->ClearChildren();
	FilterButtonBox->SetInnerSlotPadding(FVector2D(6.f, 6.f));
	FilterProxies.Reset();
	FilterButtons.Reset();
	FilterButtonLabels.Reset();
	SourceComboBox = nullptr;
	TargetComboBox = nullptr;
	DebugModeCheckBox = nullptr;
	TimeWindowSlider = nullptr;
	TimeWindowText = nullptr;

	const int32 EffectiveFilterFontSize = GetEffectiveFilterFontSize();

	auto StyleText = [this, EffectiveFilterFontSize](UTextBlock* Text, FLinearColor Color, bool bCenter = false)
	{
		Text->SetColorAndOpacity(FSlateColor(Color));
		Text->SetShadowOffset(FVector2D(1.f, 1.f));
		Text->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.65f));
		if (bCenter)
		{
			Text->SetJustification(ETextJustify::Center);
		}

		FSlateFontInfo Font = Text->GetFont();
		Font.Size = EffectiveFilterFontSize;
		Text->SetFont(Font);
	};

	auto AddControlLabel = [this, &StyleText](const TCHAR* LabelText)
	{
		UTextBlock* Label = NewObject<UTextBlock>(this);
		Label->SetText(FText::FromString(LabelText));
		StyleText(Label, ControlTextColor);
		FilterButtonBox->AddChild(Label);
		return Label;
	};

	auto AddSizedChild = [this](UWidget* Child, float Width, float Height)
	{
		USizeBox* Box = NewObject<USizeBox>(this);
		Box->SetWidthOverride(Width);
		Box->SetHeightOverride(Height);
		Box->SetContent(Child);
		FilterButtonBox->AddChild(Box);
		return Box;
	};

	struct FEntry { ECombatLogFilter Filter; const TCHAR* Label; };
	static const FEntry Entries[] =
	{
		{ ECombatLogFilter::All,      TEXT("全部") },
		{ ECombatLogFilter::Normal,   TEXT("普通") },
		{ ECombatLogFilter::Crit,     TEXT("暴击") },
		{ ECombatLogFilter::Rune,     TEXT("符文") },
		{ ECombatLogFilter::Bleed,    TEXT("流血") },
		{ ECombatLogFilter::Card,     TEXT("卡牌") },
		{ ECombatLogFilter::Finisher, TEXT("终结技") },
		{ ECombatLogFilter::Link,     TEXT("连携") },
		{ ECombatLogFilter::Shuffle,  TEXT("洗牌") },
	};

	for (const FEntry& E : Entries)
	{
		UCombatFilterProxy* Proxy = NewObject<UCombatFilterProxy>(this);
		Proxy->Filter = E.Filter;
		Proxy->Owner = this;
		FilterProxies.Add(Proxy);

		UButton* Btn = NewObject<UButton>(this);
		UTextBlock* Lbl = NewObject<UTextBlock>(this);
		Lbl->SetText(FText::FromString(E.Label));
		StyleText(Lbl, GetFilterTint(E.Filter), true);

		Btn->AddChild(Lbl);
		Btn->OnClicked.AddDynamic(Proxy, &UCombatFilterProxy::OnClicked);
		AddSizedChild(Btn, E.Filter == ECombatLogFilter::Finisher ? 76.f : 58.f, 28.f);

		FilterButtons.Add(E.Filter, Btn);
		FilterButtonLabels.Add(E.Filter, Lbl);
	}

	AddControlLabel(TEXT("攻击者"));
	SourceComboBox = NewObject<UComboBoxString>(this);
	SourceComboBox->OnSelectionChanged.AddDynamic(this, &UCombatLogWidget::OnSourceFilterChanged);
	AddSizedChild(SourceComboBox, 158.f, 30.f);

	AddControlLabel(TEXT("目标"));
	TargetComboBox = NewObject<UComboBoxString>(this);
	TargetComboBox->OnSelectionChanged.AddDynamic(this, &UCombatLogWidget::OnTargetFilterChanged);
	AddSizedChild(TargetComboBox, 158.f, 30.f);

	AddControlLabel(TEXT("调试"));
	DebugModeCheckBox = NewObject<UCheckBox>(this);
	DebugModeCheckBox->SetIsChecked(bDebugMode);
	DebugModeCheckBox->OnCheckStateChanged.AddDynamic(this, &UCombatLogWidget::OnDebugModeChanged);
	AddSizedChild(DebugModeCheckBox, 28.f, 28.f);

	TimeWindowText = NewObject<UTextBlock>(this);
	StyleText(TimeWindowText, ControlTextColor);
	FilterButtonBox->AddChild(TimeWindowText);

	TimeWindowSlider = NewObject<USlider>(this);
	TimeWindowSlider->SetSliderBarColor(FLinearColor(0.16f, 0.24f, 0.26f, 1.f));
	TimeWindowSlider->SetSliderHandleColor(FLinearColor(0.75f, 0.93f, 0.92f, 1.f));
	TimeWindowSlider->SetStepSize(0.025f);
	const float SafeMaxWindow = FMath::Max(1.f, MaxTimeWindowSeconds);
	TimeWindowSlider->SetValue(ActiveTimeWindowSeconds <= 0.f ? 0.f : FMath::Clamp(ActiveTimeWindowSeconds / SafeMaxWindow, 0.f, 1.f));
	TimeWindowSlider->OnValueChanged.AddDynamic(this, &UCombatLogWidget::OnTimeWindowSliderChanged);
	AddSizedChild(TimeWindowSlider, 180.f, 28.f);

	RefreshFilterButtonVisuals();
}

void UCombatLogWidget::BuildFilterButtonsLegacy()
{
	if (!FilterButtonBox) return;
	FilterButtonBox->ClearChildren();
	FilterProxies.Reset();
	SourceComboBox = nullptr;
	TargetComboBox = nullptr;
	DebugModeCheckBox = nullptr;
	TimeWindowSlider = nullptr;
	TimeWindowText = nullptr;

	auto AddControlLabel = [this](const TCHAR* LabelText)
	{
		UTextBlock* Label = NewObject<UTextBlock>(this);
		Label->SetText(FText::FromString(LabelText));
		Label->SetColorAndOpacity(FSlateColor(FLinearColor(0.82f, 0.88f, 0.9f, 1.f)));
		FSlateFontInfo Font = Label->GetFont();
		Font.Size = FilterFontSize;
		Label->SetFont(Font);
		FilterButtonBox->AddChild(Label);
		return Label;
	};

	struct FEntry { ECombatLogFilter Filter; const TCHAR* Label; FLinearColor Color; };
	static const FEntry Entries[] =
	{
		{ ECombatLogFilter::All,      TEXT("全部"),   { 1.f,    1.f,   1.f,  1.f } },
		{ ECombatLogFilter::Normal,   TEXT("普通"),   { 0.7f,   0.7f,  0.7f, 1.f } },
		{ ECombatLogFilter::Crit,     TEXT("暴击"),   { 1.f,    0.9f,  0.1f, 1.f } },
		{ ECombatLogFilter::Rune,     TEXT("符文"),   { 0.8f,   0.4f,  1.f,  1.f } },
		{ ECombatLogFilter::Bleed,    TEXT("流血"),   { 1.f,    0.2f,  0.2f, 1.f } },
		{ ECombatLogFilter::Card,     TEXT("卡牌"),   { 0.4f,   0.7f,  1.f,  1.f } },
		{ ECombatLogFilter::Finisher, TEXT("终结技"), { 1.f,    0.85f, 0.f,  1.f } },
		{ ECombatLogFilter::Link,     TEXT("连携"),   { 1.f,    0.55f, 0.1f, 1.f } },
		{ ECombatLogFilter::Shuffle,  TEXT("洗牌"),   { 0.5f,   0.7f,  1.f,  1.f } },
	};

	for (const FEntry& E : Entries)
	{
		UCombatFilterProxy* Proxy = NewObject<UCombatFilterProxy>(this);
		Proxy->Filter = E.Filter;
		Proxy->Owner  = this;
		FilterProxies.Add(Proxy);

		UButton*    Btn = NewObject<UButton>(this);
		UTextBlock* Lbl = NewObject<UTextBlock>(this);

		Lbl->SetText(FText::FromString(E.Label));
		Lbl->SetColorAndOpacity(FSlateColor(E.Color));

		FSlateFontInfo Font = Lbl->GetFont();
		Font.Size = FilterFontSize;
		Lbl->SetFont(Font);

		Btn->AddChild(Lbl);
		Btn->OnClicked.AddDynamic(Proxy, &UCombatFilterProxy::OnClicked);
		FilterButtonBox->AddChild(Btn);
	}

	AddControlLabel(TEXT(" 攻击者:"));
	SourceComboBox = NewObject<UComboBoxString>(this);
	SourceComboBox->OnSelectionChanged.AddDynamic(this, &UCombatLogWidget::OnSourceFilterChanged);
	USizeBox* SourceComboBoxBox = NewObject<USizeBox>(this);
	SourceComboBoxBox->SetWidthOverride(150.f);
	SourceComboBoxBox->SetContent(SourceComboBox);
	FilterButtonBox->AddChild(SourceComboBoxBox);

	AddControlLabel(TEXT(" 目标:"));
	TargetComboBox = NewObject<UComboBoxString>(this);
	TargetComboBox->OnSelectionChanged.AddDynamic(this, &UCombatLogWidget::OnTargetFilterChanged);
	USizeBox* TargetComboBoxBox = NewObject<USizeBox>(this);
	TargetComboBoxBox->SetWidthOverride(150.f);
	TargetComboBoxBox->SetContent(TargetComboBox);
	FilterButtonBox->AddChild(TargetComboBoxBox);

	AddControlLabel(TEXT(" 调试:"));
	DebugModeCheckBox = NewObject<UCheckBox>(this);
	DebugModeCheckBox->SetIsChecked(bDebugMode);
	DebugModeCheckBox->OnCheckStateChanged.AddDynamic(this, &UCombatLogWidget::OnDebugModeChanged);
	FilterButtonBox->AddChild(DebugModeCheckBox);

	TimeWindowText = NewObject<UTextBlock>(this);
	TimeWindowText->SetColorAndOpacity(FSlateColor(FLinearColor(0.82f, 0.88f, 0.9f, 1.f)));
	FSlateFontInfo TimeFont = TimeWindowText->GetFont();
	TimeFont.Size = FilterFontSize;
	TimeWindowText->SetFont(TimeFont);
	FilterButtonBox->AddChild(TimeWindowText);

	TimeWindowSlider = NewObject<USlider>(this);
	const float SafeMaxWindow = FMath::Max(1.f, MaxTimeWindowSeconds);
	TimeWindowSlider->SetValue(ActiveTimeWindowSeconds <= 0.f ? 0.f : FMath::Clamp(ActiveTimeWindowSeconds / SafeMaxWindow, 0.f, 1.f));
	TimeWindowSlider->OnValueChanged.AddDynamic(this, &UCombatLogWidget::OnTimeWindowSliderChanged);
	USizeBox* TimeWindowSliderBox = NewObject<USizeBox>(this);
	TimeWindowSliderBox->SetWidthOverride(180.f);
	TimeWindowSliderBox->SetContent(TimeWindowSlider);
	FilterButtonBox->AddChild(TimeWindowSliderBox);
}

void UCombatLogWidget::RefreshFilterButtonVisuals()
{
	for (const TPair<ECombatLogFilter, TObjectPtr<UButton>>& Pair : FilterButtons)
	{
		const bool bActive = Pair.Key == ActiveFilter;
		if (Pair.Value)
		{
			Pair.Value->SetBackgroundColor(bActive
				? FLinearColor(0.1f, 0.2f, 0.22f, 1.f)
				: FLinearColor(0.035f, 0.043f, 0.048f, 1.f));
			Pair.Value->SetColorAndOpacity(FLinearColor(1.f, 1.f, 1.f, bActive ? 1.f : 0.82f));
		}

		if (TObjectPtr<UTextBlock>* Label = FilterButtonLabels.Find(Pair.Key))
		{
			if (*Label)
			{
				const FLinearColor BaseColor = GetFilterTint(Pair.Key);
				(*Label)->SetColorAndOpacity(FSlateColor(bActive ? BaseColor : FLinearColor(BaseColor.R, BaseColor.G, BaseColor.B, 0.72f)));
			}
		}
	}
}

void UCombatLogWidget::RefreshFilterOptions()
{
	if (!SourceComboBox || !TargetComboBox)
	{
		return;
	}

	bRefreshingFilterOptions = true;

	const FString PreviousSource = ActiveSourceFilter.IsEmpty() ? FString(TEXT("全部")) : ActiveSourceFilter;
	const FString PreviousTarget = ActiveTargetFilter.IsEmpty() ? FString(TEXT("全部")) : ActiveTargetFilter;
	TSet<FString> Sources;
	TSet<FString> Targets;

	for (const FDamageBreakdown& Entry : UCombatLogStatics::GetAllEntries())
	{
		Sources.Add(UCombatLogStatics::GetDisplayActorName(Entry.SourceName));
		if (!Entry.TargetName.IsEmpty())
		{
			Targets.Add(UCombatLogStatics::GetDisplayActorName(Entry.TargetName));
		}
	}

	TArray<FString> SortedSources = Sources.Array();
	TArray<FString> SortedTargets = Targets.Array();
	SortedSources.Sort();
	SortedTargets.Sort();

	SourceComboBox->ClearOptions();
	TargetComboBox->ClearOptions();
	SourceComboBox->AddOption(TEXT("全部"));
	TargetComboBox->AddOption(TEXT("全部"));
	for (const FString& Source : SortedSources)
	{
		if (!Source.IsEmpty())
		{
			SourceComboBox->AddOption(Source);
		}
	}
	for (const FString& Target : SortedTargets)
	{
		if (!Target.IsEmpty())
		{
			TargetComboBox->AddOption(Target);
		}
	}

	ActiveSourceFilter = SourceComboBox->FindOptionIndex(PreviousSource) != INDEX_NONE ? PreviousSource : TEXT("全部");
	ActiveTargetFilter = TargetComboBox->FindOptionIndex(PreviousTarget) != INDEX_NONE ? PreviousTarget : TEXT("全部");
	SourceComboBox->SetSelectedOption(ActiveSourceFilter);
	TargetComboBox->SetSelectedOption(ActiveTargetFilter);

	bRefreshingFilterOptions = false;
}

void UCombatLogWidget::RefreshTimeWindowText()
{
	if (!TimeWindowText)
	{
		return;
	}

	const FString Label = ActiveTimeWindowSeconds <= 0.f
		? TEXT("时间: 全部")
		: FString::Printf(TEXT("时间: %.0fs"), ActiveTimeWindowSeconds);
	TimeWindowText->SetText(FText::FromString(Label));
}

void UCombatLogWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	const int32 CurrentVersion = UCombatLogStatics::GetVersion();
	if (CurrentVersion == CachedVersion)
		return;

	CachedVersion = CurrentVersion;
	RefreshFilterOptions();
	RebuildLog();
	RefreshSummary();
}

void UCombatLogWidget::SetFilter(ECombatLogFilter NewFilter)
{
	if (ActiveFilter == NewFilter)
		return;
	ActiveFilter = NewFilter;
	RefreshFilterButtonVisuals();
	RebuildLog();
}

bool UCombatLogWidget::PassesFilter(const FDamageBreakdown& Entry) const
{
	return UCombatLogStatics::PassesAdvancedFilter(
		Entry,
		ActiveFilter,
		ActiveSourceFilter,
		ActiveTargetFilter,
		GetLatestEntryTime(),
		ActiveTimeWindowSeconds);
}

void UCombatLogWidget::ResetLog()
{
	UCombatLogStatics::ClearEntries();
	SessionNormal = SessionCrit = SessionRune = SessionBleed = 0.f;
	HitNormal = HitCrit = HitRune = HitBleed = 0;
	HitCardConsume = HitCardHit = HitCardMatched = 0;
	HitCardLink = HitCardFinisher = HitCardShuffle = 0;
	SessionCardHit = SessionCardLink = SessionCardFinisher = 0.f;
	if (LogScrollBox) LogScrollBox->ClearChildren();
	RefreshFilterOptions();
	RefreshSummary();
}

void UCombatLogWidget::RebuildLog()
{
	if (!LogScrollBox)
		return;

	SessionNormal = SessionCrit = SessionRune = SessionBleed = 0.f;
	HitNormal = HitCrit = HitRune = HitBleed = 0;
	HitCardConsume = HitCardHit = HitCardMatched = 0;
	HitCardLink = HitCardFinisher = HitCardShuffle = 0;
	SessionCardHit = SessionCardLink = SessionCardFinisher = 0.f;
	LogScrollBox->ClearChildren();

	for (const FDamageBreakdown& E : UCombatLogStatics::GetAllEntries())
	{
		const FString TypeStr = E.DamageType.ToString();

		if (E.DamageType == FName("Card_Consume") || E.DamageType == FName("Card_Shuffle"))
		{
			HitCardConsume++;
			if (E.bStartedShuffle) HitCardShuffle++;
		}
		else if (E.DamageType == FName("Card_Finisher"))
		{
			HitCardFinisher++;
			HitCardHit++;
			SessionCardFinisher += E.FinalDamage;
		}
		else if (E.DamageType == FName("Card_Link"))
		{
			HitCardLink++;
			HitCardHit++;
			SessionCardLink += E.FinalDamage;
		}
		else if (E.DamageType == FName("Card_Matched"))
		{
			HitCardMatched++;
			HitCardHit++;
			SessionCardHit += E.FinalDamage;
		}
		else if (E.DamageType == FName("Card_Hit"))
		{
			HitCardHit++;
			SessionCardHit += E.FinalDamage;
		}
		else if (TypeStr.StartsWith("Rune"))             { SessionRune   += E.FinalDamage; HitRune++;   }
		else if (E.DamageType == FName("Bleed"))         { SessionBleed  += E.FinalDamage; HitBleed++;  }
		else if (E.bIsCrit)                              { SessionCrit   += E.FinalDamage; HitCrit++;   }
		else                                             { SessionNormal += E.FinalDamage; HitNormal++; }

		if (PassesFilter(E))
			AddLogRow(E);
	}
}

void UCombatLogWidget::AddLogRow(const FDamageBreakdown& Entry)
{
	if (!LogScrollBox) return;

	const float OffsetEnd    = LogScrollBox->GetScrollOffsetOfEnd();
	const float OffsetNow    = LogScrollBox->GetScrollOffset();
	const bool  bWasAtBottom = (OffsetEnd - OffsetNow) < 4.f;

	UBorder* RowBorder = NewObject<UBorder>(LogScrollBox);
	RowBorder->SetPadding(LogRowPadding);
	FLinearColor EffectiveRowBackground = LogRowBackgroundColor;
	EffectiveRowBackground.A = FMath::Max(EffectiveRowBackground.A, 0.62f);
	RowBorder->SetBrushColor(EffectiveRowBackground);

	UWrapBox* RowBox = NewObject<UWrapBox>(RowBorder);
	RowBox->SetInnerSlotPadding(FVector2D(0.f, 1.f));
	RowBorder->SetContent(RowBox);

	const int32 EffectiveLogFontSize = GetEffectiveLogFontSize();
	for (const FCombatLogTextSegment& Segment : UCombatLogStatics::GetEntryTextSegments(Entry, bDebugMode))
	{
		UTextBlock* SegmentText = NewObject<UTextBlock>(RowBox);
		SegmentText->SetText(FText::FromString(Segment.Text));
		SegmentText->SetColorAndOpacity(FSlateColor(Segment.Color));
		SegmentText->SetShadowOffset(FVector2D(1.f, 1.f));
		SegmentText->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.75f));

		FSlateFontInfo FontInfo = SegmentText->GetFont();
		FontInfo.Size = EffectiveLogFontSize;
		SegmentText->SetFont(FontInfo);
		RowBox->AddChild(SegmentText);
	}

	LogScrollBox->AddChild(RowBorder);
	if (bWasAtBottom) LogScrollBox->ScrollToEnd();
}

FString UCombatLogWidget::FormatEntry(const FDamageBreakdown& Entry) const
{
	return UCombatLogStatics::GetEntryText(Entry);
}

FLinearColor UCombatLogWidget::GetEntryColor(const FDamageBreakdown& Entry) const
{
	return UCombatLogStatics::GetEntryColor(Entry);
}

float UCombatLogWidget::GetLatestEntryTime() const
{
	float LatestTime = 0.f;
	for (const FDamageBreakdown& Entry : UCombatLogStatics::GetAllEntries())
	{
		LatestTime = FMath::Max(LatestTime, Entry.GameTime);
	}
	return LatestTime;
}

void UCombatLogWidget::OnSourceFilterChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	if (bRefreshingFilterOptions)
	{
		return;
	}

	ActiveSourceFilter = SelectedItem.IsEmpty() ? FString(TEXT("全部")) : SelectedItem;
	RebuildLog();
}

void UCombatLogWidget::OnTargetFilterChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	if (bRefreshingFilterOptions)
	{
		return;
	}

	ActiveTargetFilter = SelectedItem.IsEmpty() ? FString(TEXT("全部")) : SelectedItem;
	RebuildLog();
}

void UCombatLogWidget::OnDebugModeChanged(bool bIsChecked)
{
	bDebugMode = bIsChecked;
	RebuildLog();
}

void UCombatLogWidget::OnTimeWindowSliderChanged(float Value)
{
	const float SafeMaxWindow = FMath::Max(1.f, MaxTimeWindowSeconds);
	ActiveTimeWindowSeconds = Value <= 0.01f ? 0.f : FMath::RoundToFloat(Value * SafeMaxWindow);
	RefreshTimeWindowText();
	RebuildLog();
}

void UCombatLogWidget::RefreshSummary()
{
	if (!SummaryText)
	{
		return;
	}

	ApplySummaryTextStyle();

	const float Total = SessionNormal + SessionCrit + SessionRune + SessionBleed
		+ SessionCardHit + SessionCardLink + SessionCardFinisher;

	SummaryText->SetText(FText::FromString(FString::Printf(
		TEXT("卡牌统计  消耗 %d次  命中 %d次  匹配 %d次  连携 %d次  终结技 %d次  洗牌 %d次\n")
		TEXT("伤害统计  普通 %.0f(%d次)  暴击 %.0f(%d次)  符文 %.0f(%d次)  流血 %.0f(%d次)\n")
		TEXT("卡牌伤害  命中 %.0f(%d次)  终结技 %.0f(%d次)  连携 %.0f(%d次)  总计 %.0f"),
		HitCardConsume, HitCardHit, HitCardMatched, HitCardLink, HitCardFinisher, HitCardShuffle,
		SessionNormal, HitNormal, SessionCrit, HitCrit, SessionRune, HitRune, SessionBleed, HitBleed,
		SessionCardHit, HitCardHit, SessionCardFinisher, HitCardFinisher, SessionCardLink, HitCardLink,
		Total)));
}

void UCombatLogWidget::RefreshSummaryLegacy()
{
	if (!SummaryText) return;

	const float Total = SessionNormal + SessionCrit + SessionRune + SessionBleed
	                  + SessionCardHit + SessionCardLink + SessionCardFinisher;

	SummaryText->SetText(FText::FromString(FString::Printf(
		TEXT("-- 卡牌统计 --\n")
		TEXT("消耗: %d次 | 命中: %d次 | 匹配: %d次 | 连携: %d次 | 终结技: %d次 | 洗牌: %d次\n")
		TEXT("-- 伤害统计 --\n")
		TEXT("普通: %.0f(%d次) | 暴击: %.0f(%d次) | 符文: %.0f(%d次) | 流血: %.0f(%d次)\n")
		TEXT("卡牌: %.0f(%d次) | 终结技: %.0f(%d次) | 连携: %.0f(%d次)\n")
		TEXT("总计: %.0f"),
		HitCardConsume, HitCardHit, HitCardMatched, HitCardLink, HitCardFinisher, HitCardShuffle,
		SessionNormal, HitNormal, SessionCrit, HitCrit, SessionRune, HitRune, SessionBleed, HitBleed,
		SessionCardHit, HitCardHit, SessionCardFinisher, HitCardFinisher, SessionCardLink, HitCardLink,
		Total)));
}
