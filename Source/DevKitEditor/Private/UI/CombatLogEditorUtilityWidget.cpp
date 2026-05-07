#include "UI/CombatLogEditorUtilityWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/CheckBox.h"
#include "Components/ComboBoxString.h"
#include "Components/PanelWidget.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Components/WrapBox.h"

namespace
{
constexpr float OuterPadding = 12.f;
constexpr float TopPadding = 10.f;
constexpr float PanelGap = 8.f;

FLinearColor GetFilterTint(ECombatLogFilter Filter)
{
	switch (Filter)
	{
	case ECombatLogFilter::Normal:   return FLinearColor(0.72f, 0.76f, 0.76f, 1.f);
	case ECombatLogFilter::Crit:     return FLinearColor(1.f, 0.78f, 0.18f, 1.f);
	case ECombatLogFilter::Rune:     return FLinearColor(0.78f, 0.48f, 1.f, 1.f);
	case ECombatLogFilter::Bleed:    return FLinearColor(1.f, 0.24f, 0.24f, 1.f);
	case ECombatLogFilter::Card:     return FLinearColor(0.42f, 0.70f, 1.f, 1.f);
	case ECombatLogFilter::Finisher: return FLinearColor(1.f, 0.84f, 0.20f, 1.f);
	case ECombatLogFilter::Link:     return FLinearColor(1.f, 0.55f, 0.16f, 1.f);
	case ECombatLogFilter::Shuffle:  return FLinearColor(0.52f, 0.72f, 1.f, 1.f);
	default:                         return FLinearColor(0.90f, 0.96f, 0.96f, 1.f);
	}
}
}

void UCombatLogEditorFilterProxy::OnClicked()
{
	if (Owner.IsValid())
	{
		Owner->SetFilter(Filter);
	}
}

TSharedRef<SWidget> UCombatLogEditorUtilityWidget::RebuildWidget()
{
	EnsureLayout();
	return Super::RebuildWidget();
}

void UCombatLogEditorUtilityWidget::NativeConstruct()
{
	Super::NativeConstruct();

	ActiveTimeWindowSeconds = DefaultTimeWindowSeconds;
	bDebugMode = bDefaultDebugMode;

	EnsureLayout();
	BuildControlBar();
	RefreshFilterOptions();
	RefreshTimeWindowText();
	RebuildLog();
	RefreshSummary();
}

void UCombatLogEditorUtilityWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	const int32 CurrentVersion = UCombatLogStatics::GetVersion();
	if (CurrentVersion == CachedVersion)
	{
		return;
	}

	CachedVersion = CurrentVersion;
	RefreshFilterOptions();
	RebuildLog();
	RefreshSummary();
}

int32 UCombatLogEditorUtilityWidget::GetSafeLogFontSize() const
{
	return FMath::Clamp(LogFontSize, 10, 16);
}

int32 UCombatLogEditorUtilityWidget::GetSafeFilterFontSize() const
{
	return FMath::Clamp(FilterFontSize, 9, 14);
}

int32 UCombatLogEditorUtilityWidget::GetSafeSummaryFontSize() const
{
	return FMath::Clamp(SummaryFontSize, 9, 13);
}

void UCombatLogEditorUtilityWidget::EnsureLayout()
{
	if (!WidgetTree)
	{
		return;
	}

	UCanvasPanel* Canvas = Cast<UCanvasPanel>(WidgetTree->RootWidget);
	if (!Canvas)
	{
		Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), FName(TEXT("RootCanvas")));
		WidgetTree->RootWidget = Canvas;
	}

	if (!WidgetTree->FindWidget(FName(TEXT("CombatLogEditorBackdrop"))))
	{
		UBorder* Backdrop = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), FName(TEXT("CombatLogEditorBackdrop")));
		Backdrop->SetBrushColor(FLinearColor(0.015f, 0.018f, 0.022f, 0.96f));
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

	if (!FilterButtonBox)
	{
		FilterButtonBox = Cast<UWrapBox>(WidgetTree->FindWidget(FName(TEXT("FilterButtonBox"))));
	}
	if (!FilterButtonBox)
	{
		FilterButtonBox = WidgetTree->ConstructWidget<UWrapBox>(UWrapBox::StaticClass(), FName(TEXT("FilterButtonBox")));
		Canvas->AddChildToCanvas(FilterButtonBox);
	}

	if (!LogScrollBox)
	{
		LogScrollBox = Cast<UScrollBox>(WidgetTree->FindWidget(FName(TEXT("LogScrollBox"))));
	}
	if (!LogScrollBox)
	{
		LogScrollBox = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), FName(TEXT("LogScrollBox")));
		Canvas->AddChildToCanvas(LogScrollBox);
	}

	if (!SummaryText)
	{
		SummaryText = Cast<UTextBlock>(WidgetTree->FindWidget(FName(TEXT("SummaryText"))));
	}
	if (!SummaryText)
	{
		SummaryText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), FName(TEXT("SummaryText")));
		Canvas->AddChildToCanvas(SummaryText);
	}

	const float SafeControlHeight = FMath::Clamp(ControlBarHeight, 76.f, 140.f);
	const float SafeSummaryHeight = FMath::Clamp(SummaryHeight, 84.f, 180.f);
	const float LogTop = TopPadding + SafeControlHeight + PanelGap;
	const float LogBottom = SafeSummaryHeight + OuterPadding + PanelGap;

	FilterButtonBox->SetInnerSlotPadding(FVector2D(6.f, 6.f));
	if (UCanvasPanelSlot* FilterSlot = Cast<UCanvasPanelSlot>(FilterButtonBox->Slot))
	{
		FilterSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 0.f));
		FilterSlot->SetPosition(FVector2D(OuterPadding, TopPadding));
		FilterSlot->SetSize(FVector2D(-OuterPadding, SafeControlHeight));
		FilterSlot->SetAutoSize(false);
		FilterSlot->SetZOrder(20);
	}

	LogScrollBox->SetClipping(EWidgetClipping::ClipToBounds);
	if (UCanvasPanelSlot* LogSlot = Cast<UCanvasPanelSlot>(LogScrollBox->Slot))
	{
		LogSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
		LogSlot->SetPosition(FVector2D(OuterPadding, LogTop));
		LogSlot->SetSize(FVector2D(-OuterPadding, -LogBottom));
		LogSlot->SetAutoSize(false);
		LogSlot->SetZOrder(10);
	}

	SummaryText->SetAutoWrapText(true);
	SummaryText->SetColorAndOpacity(FSlateColor(FLinearColor(0.64f, 0.72f, 0.73f, 1.f)));
	SummaryText->SetShadowOffset(FVector2D(1.f, 1.f));
	SummaryText->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.7f));
	FSlateFontInfo SummaryFont = SummaryText->GetFont();
	SummaryFont.Size = GetSafeSummaryFontSize();
	SummaryText->SetFont(SummaryFont);
	if (UCanvasPanelSlot* SummarySlot = Cast<UCanvasPanelSlot>(SummaryText->Slot))
	{
		SummarySlot->SetAnchors(FAnchors(0.f, 1.f, 1.f, 1.f));
		SummarySlot->SetPosition(FVector2D(OuterPadding, -SafeSummaryHeight - OuterPadding));
		SummarySlot->SetSize(FVector2D(-OuterPadding, SafeSummaryHeight));
		SummarySlot->SetAutoSize(false);
		SummarySlot->SetZOrder(15);
	}
}

void UCombatLogEditorUtilityWidget::BuildControlBar()
{
	if (!FilterButtonBox)
	{
		return;
	}

	FilterButtonBox->ClearChildren();
	FilterProxies.Reset();
	FilterButtons.Reset();
	FilterButtonLabels.Reset();

	const int32 SafeFilterFont = GetSafeFilterFontSize();

	auto StyleText = [SafeFilterFont](UTextBlock* Text, const FLinearColor& Color, bool bCenter = false)
	{
		Text->SetColorAndOpacity(FSlateColor(Color));
		Text->SetShadowOffset(FVector2D(1.f, 1.f));
		Text->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.65f));
		if (bCenter)
		{
			Text->SetJustification(ETextJustify::Center);
		}
		FSlateFontInfo Font = Text->GetFont();
		Font.Size = SafeFilterFont;
		Text->SetFont(Font);
	};

	auto AddSizedChild = [this](UWidget* Child, float Width, float Height)
	{
		USizeBox* Box = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		Box->SetWidthOverride(Width);
		Box->SetHeightOverride(Height);
		Box->SetContent(Child);
		FilterButtonBox->AddChild(Box);
	};

	auto AddLabel = [this, &StyleText](const TCHAR* LabelText)
	{
		UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Label->SetText(FText::FromString(LabelText));
		StyleText(Label, FLinearColor(0.78f, 0.84f, 0.86f, 1.f));
		FilterButtonBox->AddChild(Label);
	};

	struct FFilterEntry
	{
		ECombatLogFilter Filter;
		const TCHAR* Label;
		float Width;
	};

	static const FFilterEntry Entries[] =
	{
		{ ECombatLogFilter::All, TEXT("全部"), 58.f },
		{ ECombatLogFilter::Normal, TEXT("普通"), 58.f },
		{ ECombatLogFilter::Crit, TEXT("暴击"), 58.f },
		{ ECombatLogFilter::Rune, TEXT("符文"), 58.f },
		{ ECombatLogFilter::Bleed, TEXT("流血"), 58.f },
		{ ECombatLogFilter::Card, TEXT("卡牌"), 58.f },
		{ ECombatLogFilter::Finisher, TEXT("终结技"), 76.f },
		{ ECombatLogFilter::Link, TEXT("连携"), 58.f },
		{ ECombatLogFilter::Shuffle, TEXT("洗牌"), 58.f },
	};

	for (const FFilterEntry& Entry : Entries)
	{
		UCombatLogEditorFilterProxy* Proxy = NewObject<UCombatLogEditorFilterProxy>(this);
		Proxy->Filter = Entry.Filter;
		Proxy->Owner = this;
		FilterProxies.Add(Proxy);

		UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
		UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Label->SetText(FText::FromString(Entry.Label));
		StyleText(Label, GetFilterTint(Entry.Filter), true);
		Button->SetContent(Label);
		Button->OnClicked.AddDynamic(Proxy, &UCombatLogEditorFilterProxy::OnClicked);

		AddSizedChild(Button, Entry.Width, 28.f);
		FilterButtons.Add(Entry.Filter, Button);
		FilterButtonLabels.Add(Entry.Filter, Label);
	}

	AddLabel(TEXT("攻击者"));
	SourceComboBox = WidgetTree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass());
	SourceComboBox->OnSelectionChanged.AddDynamic(this, &UCombatLogEditorUtilityWidget::OnSourceFilterChanged);
	AddSizedChild(SourceComboBox, 158.f, 30.f);

	AddLabel(TEXT("目标"));
	TargetComboBox = WidgetTree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass());
	TargetComboBox->OnSelectionChanged.AddDynamic(this, &UCombatLogEditorUtilityWidget::OnTargetFilterChanged);
	AddSizedChild(TargetComboBox, 158.f, 30.f);

	AddLabel(TEXT("调试"));
	DebugModeCheckBox = WidgetTree->ConstructWidget<UCheckBox>(UCheckBox::StaticClass());
	DebugModeCheckBox->SetIsChecked(bDebugMode);
	DebugModeCheckBox->OnCheckStateChanged.AddDynamic(this, &UCombatLogEditorUtilityWidget::OnDebugModeChanged);
	AddSizedChild(DebugModeCheckBox, 28.f, 28.f);

	TimeWindowText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	StyleText(TimeWindowText, FLinearColor(0.78f, 0.84f, 0.86f, 1.f));
	FilterButtonBox->AddChild(TimeWindowText);

	TimeWindowSlider = WidgetTree->ConstructWidget<USlider>(USlider::StaticClass());
	TimeWindowSlider->SetSliderBarColor(FLinearColor(0.16f, 0.24f, 0.26f, 1.f));
	TimeWindowSlider->SetSliderHandleColor(FLinearColor(0.75f, 0.93f, 0.92f, 1.f));
	TimeWindowSlider->SetStepSize(0.025f);
	const float SafeMaxWindow = FMath::Max(1.f, MaxTimeWindowSeconds);
	TimeWindowSlider->SetValue(ActiveTimeWindowSeconds <= 0.f ? 0.f : FMath::Clamp(ActiveTimeWindowSeconds / SafeMaxWindow, 0.f, 1.f));
	TimeWindowSlider->OnValueChanged.AddDynamic(this, &UCombatLogEditorUtilityWidget::OnTimeWindowSliderChanged);
	AddSizedChild(TimeWindowSlider, 180.f, 28.f);

	RefreshFilterButtonVisuals();
}

void UCombatLogEditorUtilityWidget::RefreshFilterButtonVisuals()
{
	for (const TPair<ECombatLogFilter, TObjectPtr<UButton>>& Pair : FilterButtons)
	{
		const bool bActive = Pair.Key == ActiveFilter;
		if (Pair.Value)
		{
			Pair.Value->SetBackgroundColor(bActive
				? FLinearColor(0.10f, 0.20f, 0.22f, 1.f)
				: FLinearColor(0.035f, 0.043f, 0.048f, 1.f));
			Pair.Value->SetColorAndOpacity(FLinearColor(1.f, 1.f, 1.f, bActive ? 1.f : 0.82f));
		}

		if (TObjectPtr<UTextBlock>* Label = FilterButtonLabels.Find(Pair.Key))
		{
			const FLinearColor BaseColor = GetFilterTint(Pair.Key);
			(*Label)->SetColorAndOpacity(FSlateColor(bActive ? BaseColor : FLinearColor(BaseColor.R, BaseColor.G, BaseColor.B, 0.72f)));
		}
	}
}

void UCombatLogEditorUtilityWidget::RefreshFilterOptions()
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

void UCombatLogEditorUtilityWidget::RefreshTimeWindowText()
{
	if (!TimeWindowText)
	{
		return;
	}

	TimeWindowText->SetText(FText::FromString(ActiveTimeWindowSeconds <= 0.f
		? TEXT("时间: 全部")
		: FString::Printf(TEXT("时间: %.0fs"), ActiveTimeWindowSeconds)));
}

float UCombatLogEditorUtilityWidget::GetLatestEntryTime() const
{
	float LatestTime = 0.f;
	for (const FDamageBreakdown& Entry : UCombatLogStatics::GetAllEntries())
	{
		LatestTime = FMath::Max(LatestTime, Entry.GameTime);
	}
	return LatestTime;
}

bool UCombatLogEditorUtilityWidget::PassesFilter(const FDamageBreakdown& Entry) const
{
	return UCombatLogStatics::PassesAdvancedFilter(
		Entry,
		ActiveFilter,
		ActiveSourceFilter,
		ActiveTargetFilter,
		GetLatestEntryTime(),
		ActiveTimeWindowSeconds);
}

void UCombatLogEditorUtilityWidget::RebuildLog()
{
	if (!LogScrollBox)
	{
		return;
	}

	SessionNormal = SessionCrit = SessionRune = SessionBleed = 0.f;
	SessionCardHit = SessionCardLink = SessionCardFinisher = 0.f;
	HitNormal = HitCrit = HitRune = HitBleed = 0;
	HitCardConsume = HitCardHit = HitCardMatched = 0;
	HitCardLink = HitCardFinisher = HitCardShuffle = 0;

	LogScrollBox->ClearChildren();

	for (const FDamageBreakdown& Entry : UCombatLogStatics::GetAllEntries())
	{
		const FString TypeStr = Entry.DamageType.ToString();
		if (Entry.DamageType == FName(TEXT("Card_Consume")) || Entry.DamageType == FName(TEXT("Card_Shuffle")))
		{
			HitCardConsume++;
			if (Entry.bStartedShuffle)
			{
				HitCardShuffle++;
			}
		}
		else if (Entry.DamageType == FName(TEXT("Card_Finisher")))
		{
			HitCardFinisher++;
			HitCardHit++;
			SessionCardFinisher += Entry.FinalDamage;
		}
		else if (Entry.DamageType == FName(TEXT("Card_Link")))
		{
			HitCardLink++;
			HitCardHit++;
			SessionCardLink += Entry.FinalDamage;
		}
		else if (Entry.DamageType == FName(TEXT("Card_Matched")))
		{
			HitCardMatched++;
			HitCardHit++;
			SessionCardHit += Entry.FinalDamage;
		}
		else if (Entry.DamageType == FName(TEXT("Card_Hit")))
		{
			HitCardHit++;
			SessionCardHit += Entry.FinalDamage;
		}
		else if (TypeStr.StartsWith(TEXT("Rune"))) { SessionRune += Entry.FinalDamage; HitRune++; }
		else if (Entry.DamageType == FName(TEXT("Bleed"))) { SessionBleed += Entry.FinalDamage; HitBleed++; }
		else if (Entry.bIsCrit) { SessionCrit += Entry.FinalDamage; HitCrit++; }
		else { SessionNormal += Entry.FinalDamage; HitNormal++; }

		if (PassesFilter(Entry))
		{
			AddLogRow(Entry);
		}
	}
}

void UCombatLogEditorUtilityWidget::AddLogRow(const FDamageBreakdown& Entry)
{
	const float OffsetEnd = LogScrollBox->GetScrollOffsetOfEnd();
	const float OffsetNow = LogScrollBox->GetScrollOffset();
	const bool bWasAtBottom = (OffsetEnd - OffsetNow) < 8.f;

	UBorder* RowBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	RowBorder->SetPadding(FMargin(2.f, 1.f, 2.f, 1.f));
	RowBorder->SetBrushColor(FLinearColor(0.025f, 0.032f, 0.036f, 0.72f));

	UWrapBox* RowBox = WidgetTree->ConstructWidget<UWrapBox>(UWrapBox::StaticClass());
	RowBox->SetInnerSlotPadding(FVector2D(0.f, 1.f));
	RowBorder->SetContent(RowBox);

	for (const FCombatLogTextSegment& Segment : UCombatLogStatics::GetEntryTextSegments(Entry, bDebugMode))
	{
		UTextBlock* SegmentText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		SegmentText->SetText(FText::FromString(Segment.Text));
		SegmentText->SetColorAndOpacity(FSlateColor(Segment.Color));
		SegmentText->SetShadowOffset(FVector2D(1.f, 1.f));
		SegmentText->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.72f));

		FSlateFontInfo FontInfo = SegmentText->GetFont();
		FontInfo.Size = GetSafeLogFontSize();
		SegmentText->SetFont(FontInfo);
		RowBox->AddChild(SegmentText);
	}

	LogScrollBox->AddChild(RowBorder);
	if (bWasAtBottom)
	{
		LogScrollBox->ScrollToEnd();
	}
}

void UCombatLogEditorUtilityWidget::RefreshSummary()
{
	if (!SummaryText)
	{
		return;
	}

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

void UCombatLogEditorUtilityWidget::SetFilter(ECombatLogFilter NewFilter)
{
	if (ActiveFilter == NewFilter)
	{
		return;
	}

	ActiveFilter = NewFilter;
	RefreshFilterButtonVisuals();
	RebuildLog();
	RefreshSummary();
}

void UCombatLogEditorUtilityWidget::ResetLog()
{
	UCombatLogStatics::ClearEntries();
	if (LogScrollBox)
	{
		LogScrollBox->ClearChildren();
	}
	RefreshFilterOptions();
	RebuildLog();
	RefreshSummary();
}

void UCombatLogEditorUtilityWidget::OnSourceFilterChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	if (bRefreshingFilterOptions)
	{
		return;
	}

	ActiveSourceFilter = SelectedItem.IsEmpty() ? FString(TEXT("全部")) : SelectedItem;
	RebuildLog();
	RefreshSummary();
}

void UCombatLogEditorUtilityWidget::OnTargetFilterChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	if (bRefreshingFilterOptions)
	{
		return;
	}

	ActiveTargetFilter = SelectedItem.IsEmpty() ? FString(TEXT("全部")) : SelectedItem;
	RebuildLog();
	RefreshSummary();
}

void UCombatLogEditorUtilityWidget::OnDebugModeChanged(bool bIsChecked)
{
	bDebugMode = bIsChecked;
	RebuildLog();
}

void UCombatLogEditorUtilityWidget::OnTimeWindowSliderChanged(float Value)
{
	const float SafeMaxWindow = FMath::Max(1.f, MaxTimeWindowSeconds);
	ActiveTimeWindowSeconds = Value <= 0.01f ? 0.f : FMath::RoundToFloat(Value * SafeMaxWindow);
	RefreshTimeWindowText();
	RebuildLog();
	RefreshSummary();
}
