#include "UI/CombatLogStatics.h"

#if WITH_EDITOR
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Components/WrapBox.h"
#include "UObject/UObjectIterator.h"
#endif

// 静态成员定义
TArray<FDamageBreakdown> UCombatLogStatics::Entries;
int32 UCombatLogStatics::Version = 0;

void UCombatLogStatics::PushEntry(const FDamageBreakdown& Entry)
{
	Entries.Add(Entry);

	if (Entries.Num() > MaxEntries)
		Entries.RemoveAt(0);

	++Version;
}

int32 UCombatLogStatics::GetVersion()
{
	return Version;
}

const TArray<FDamageBreakdown>& UCombatLogStatics::GetAllEntries()
{
	return Entries;
}

int32 UCombatLogStatics::GetEntryCount()
{
	return Entries.Num();
}

void UCombatLogStatics::ClearEntries()
{
	Entries.Empty();
	++Version;
}

// ── 内部辅助：判断是否为卡牌类 DamageType ───────────────────────────────
static bool IsCardType(const FString& TypeStr)
{
	return TypeStr.StartsWith("Card_");
}

static bool IsAllFilterText(const FString& Value)
{
	return Value.IsEmpty()
		|| Value.Equals(TEXT("全部"), ESearchCase::IgnoreCase)
		|| Value.Equals(TEXT("All"), ESearchCase::IgnoreCase);
}

static FString FormatDamageValue(const float Value)
{
	return FMath::IsNearlyEqual(Value, FMath::RoundToFloat(Value), 0.05f)
		? FString::Printf(TEXT("%.0f"), Value)
		: FString::Printf(TEXT("%.1f"), Value);
}

static FString FormatCombatLogTime(const float GameTime)
{
	const int32 WholeSeconds = FMath::FloorToInt(FMath::Max(0.f, GameTime));
	const int32 Minutes = WholeSeconds / 60;
	const float Seconds = FMath::Max(0.f, GameTime) - static_cast<float>(Minutes * 60);
	return FString::Printf(TEXT("[%02d:%05.2f]"), Minutes, Seconds);
}

static void AddSegment(TArray<FCombatLogTextSegment>& Segments, const FString& Text, const FLinearColor& Color)
{
	FCombatLogTextSegment Segment;
	Segment.Text = Text;
	Segment.Color = Color;
	Segments.Add(Segment);
}

static FString BuildDebugFlags(const FDamageBreakdown& Entry)
{
	TArray<FString> Flags;
	if (Entry.bHadCard) Flags.Add(TEXT("HadCard"));
	if (Entry.bConsumedCard) Flags.Add(TEXT("Consumed"));
	if (Entry.bActionMatched) Flags.Add(TEXT("ActionMatched"));
	if (Entry.bTriggeredMatchedFlow) Flags.Add(TEXT("Matched"));
	if (Entry.bTriggeredLink) Flags.Add(TEXT("Link"));
	if (Entry.bTriggeredFinisher) Flags.Add(TEXT("Finisher"));
	if (Entry.bStartedShuffle) Flags.Add(TEXT("Shuffle"));
	if (Entry.bIsCrit) Flags.Add(TEXT("Crit"));
	return Flags.Num() > 0 ? FString::Join(Flags, TEXT(",")) : TEXT("None");
}

static FString GetCardNameForLog(const FDamageBreakdown& Entry)
{
	return Entry.CardDisplayName.IsNone()
		? FString(TEXT("卡牌"))
		: Entry.CardDisplayName.ToString();
}

#if WITH_EDITOR
static bool IsCombatLogEditorWidget(const UUserWidget* Widget)
{
	return Widget && Widget->GetClass() && Widget->GetClass()->GetName().Contains(TEXT("EUW_CombatLog"));
}

static void ApplyLegacyTextStyle(UTextBlock* TextBlock, int32 FontSize, const FLinearColor& Color)
{
	if (!TextBlock)
	{
		return;
	}

	TextBlock->SetColorAndOpacity(FSlateColor(Color));
	TextBlock->SetAutoWrapText(true);
	TextBlock->SetClipping(EWidgetClipping::ClipToBounds);
	TextBlock->SetMinDesiredWidth(0.f);
	TextBlock->SetShadowOffset(FVector2D(1.f, 1.f));
	TextBlock->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.72f));

	FSlateFontInfo FontInfo = TextBlock->GetFont();
	FontInfo.Size = FontSize;
	TextBlock->SetFont(FontInfo);
}

static FLinearColor GetLegacyFilterColor(ECombatLogFilter Filter)
{
	switch (Filter)
	{
	case ECombatLogFilter::Normal: return FLinearColor(0.72f, 0.76f, 0.76f, 1.f);
	case ECombatLogFilter::Crit:   return FLinearColor(1.f, 0.78f, 0.18f, 1.f);
	case ECombatLogFilter::Rune:   return FLinearColor(0.78f, 0.48f, 1.f, 1.f);
	case ECombatLogFilter::Bleed:  return FLinearColor(1.f, 0.24f, 0.24f, 1.f);
	default:                       return FLinearColor(0.9f, 0.96f, 0.96f, 1.f);
	}
}

static UTextBlock* FindLegacyTextBlock(UUserWidget* OwnerWidget, const TCHAR* NameHint)
{
	if (!OwnerWidget)
	{
		return nullptr;
	}

	for (TObjectIterator<UTextBlock> TextIt; TextIt; ++TextIt)
	{
		UTextBlock* TextBlock = *TextIt;
		if (!TextBlock || TextBlock->HasAnyFlags(RF_ClassDefaultObject) || TextBlock->GetTypedOuter<UUserWidget>() != OwnerWidget)
		{
			continue;
		}

		if (TextBlock->GetName().Contains(NameHint))
		{
			return TextBlock;
		}
	}

	return nullptr;
}

static UScrollBox* EnsureLegacySegmentedLogPanel(UUserWidget* OwnerWidget)
{
	if (!OwnerWidget || !OwnerWidget->WidgetTree)
	{
		return nullptr;
	}

	if (UScrollBox* ExistingScrollBox = Cast<UScrollBox>(OwnerWidget->WidgetTree->FindWidget(FName(TEXT("CombatLogLegacyScrollBox")))))
	{
		return ExistingScrollBox;
	}

	UCanvasPanel* Canvas = Cast<UCanvasPanel>(OwnerWidget->WidgetTree->RootWidget);
	if (!Canvas)
	{
		return nullptr;
	}

	UScrollBox* ScrollBox = OwnerWidget->WidgetTree->ConstructWidget<UScrollBox>(
		UScrollBox::StaticClass(),
		FName(TEXT("CombatLogLegacyScrollBox")));
	ScrollBox->SetClipping(EWidgetClipping::ClipToBounds);
	if (UCanvasPanelSlot* ScrollSlot = Canvas->AddChildToCanvas(ScrollBox))
	{
		ScrollSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
		ScrollSlot->SetPosition(FVector2D(12.f, 76.f));
		ScrollSlot->SetSize(FVector2D(-12.f, -220.f));
		ScrollSlot->SetAutoSize(false);
		ScrollSlot->SetZOrder(15);
	}

	return ScrollBox;
}

static void RebuildLegacySegmentedLog(UUserWidget* OwnerWidget, ECombatLogFilter ActiveFilter)
{
	UScrollBox* ScrollBox = EnsureLegacySegmentedLogPanel(OwnerWidget);
	if (!ScrollBox)
	{
		return;
	}

	const float OffsetEnd = ScrollBox->GetScrollOffsetOfEnd();
	const float OffsetNow = ScrollBox->GetScrollOffset();
	const bool bWasAtBottom = (OffsetEnd - OffsetNow) < 8.f;

	ScrollBox->ClearChildren();

	for (const FDamageBreakdown& Entry : UCombatLogStatics::GetAllEntries())
	{
		if (!UCombatLogStatics::PassesFilter(Entry, ActiveFilter))
		{
			continue;
		}

		UBorder* RowBorder = OwnerWidget->WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
		RowBorder->SetPadding(FMargin(2.f, 1.f, 2.f, 1.f));
		RowBorder->SetBrushColor(FLinearColor(0.025f, 0.032f, 0.036f, 0.72f));

		UWrapBox* RowBox = OwnerWidget->WidgetTree->ConstructWidget<UWrapBox>(UWrapBox::StaticClass());
		RowBox->SetInnerSlotPadding(FVector2D(0.f, 1.f));
		RowBorder->SetContent(RowBox);

		for (const FCombatLogTextSegment& Segment : UCombatLogStatics::GetEntryTextSegments(Entry, false))
		{
			UTextBlock* SegmentText = OwnerWidget->WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
			SegmentText->SetText(FText::FromString(Segment.Text));
			SegmentText->SetColorAndOpacity(FSlateColor(Segment.Color));
			SegmentText->SetShadowOffset(FVector2D(1.f, 1.f));
			SegmentText->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.72f));

			FSlateFontInfo FontInfo = SegmentText->GetFont();
			FontInfo.Size = 11;
			SegmentText->SetFont(FontInfo);
			RowBox->AddChild(SegmentText);
		}

		ScrollBox->AddChild(RowBorder);
	}

	if (ScrollBox->GetChildrenCount() == 0)
	{
		UTextBlock* EmptyText = OwnerWidget->WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		EmptyText->SetText(FText::FromString(TEXT("(暂无记录)")));
		ApplyLegacyTextStyle(EmptyText, 11, FLinearColor(0.62f, 0.68f, 0.70f, 1.f));
		ScrollBox->AddChild(EmptyText);
	}

	if (bWasAtBottom)
	{
		ScrollBox->ScrollToEnd();
	}
}

static void ApplyLegacyButtonStyle(
	UUserWidget* OwnerWidget,
	ECombatLogFilter ActiveFilter,
	const TCHAR* NameHint,
	const TCHAR* Label,
	ECombatLogFilter Filter)
{
	if (!OwnerWidget || !OwnerWidget->WidgetTree)
	{
		return;
	}

	for (TObjectIterator<UButton> ButtonIt; ButtonIt; ++ButtonIt)
	{
		UButton* Button = *ButtonIt;
		if (!Button || Button->HasAnyFlags(RF_ClassDefaultObject) || Button->GetTypedOuter<UUserWidget>() != OwnerWidget)
		{
			continue;
		}

		if (!Button->GetName().Contains(NameHint))
		{
			continue;
		}

		const bool bActive = ActiveFilter == Filter;
		Button->SetBackgroundColor(bActive
			? FLinearColor(0.10f, 0.20f, 0.22f, 1.f)
			: FLinearColor(0.035f, 0.043f, 0.048f, 1.f));
		Button->SetColorAndOpacity(FLinearColor(1.f, 1.f, 1.f, bActive ? 1.f : 0.84f));

		UTextBlock* LabelText = Cast<UTextBlock>(Button->GetContent());
		if (!LabelText)
		{
			LabelText = OwnerWidget->WidgetTree->ConstructWidget<UTextBlock>(
				UTextBlock::StaticClass(),
				*FString::Printf(TEXT("CombatLogLegacyLabel_%s"), NameHint));
			Button->SetContent(LabelText);
		}

		LabelText->SetText(FText::FromString(Label));
		LabelText->SetJustification(ETextJustify::Center);
		ApplyLegacyTextStyle(LabelText, 10, bActive ? GetLegacyFilterColor(Filter) : FLinearColor(0.68f, 0.76f, 0.78f, 1.f));

		if (UCanvasPanelSlot* ButtonSlot = Cast<UCanvasPanelSlot>(Button->Slot))
		{
			ButtonSlot->SetSize(FVector2D(Filter == ECombatLogFilter::All ? 58.f : 54.f, 28.f));
			ButtonSlot->SetAutoSize(false);
			ButtonSlot->SetZOrder(30);
		}
	}
}

static void ApplyLegacyCombatLogWidgetStyle(ECombatLogFilter ActiveFilter)
{
	TSet<TWeakObjectPtr<UUserWidget>> StyledWidgets;

	for (TObjectIterator<UTextBlock> TextIt; TextIt; ++TextIt)
	{
		UTextBlock* TextBlock = *TextIt;
		if (!TextBlock || TextBlock->HasAnyFlags(RF_ClassDefaultObject))
		{
			continue;
		}

		UUserWidget* OwnerWidget = TextBlock->GetTypedOuter<UUserWidget>();
		if (!IsCombatLogEditorWidget(OwnerWidget))
		{
			continue;
		}

		StyledWidgets.Add(OwnerWidget);

		const FString TextName = TextBlock->GetName();
		if (TextName.Contains(TEXT("LogText")))
		{
			ApplyLegacyTextStyle(TextBlock, 11, FLinearColor(0.80f, 0.84f, 0.84f, 1.f));
			TextBlock->SetVisibility(ESlateVisibility::Collapsed);
			if (UCanvasPanelSlot* LogTextSlot = Cast<UCanvasPanelSlot>(TextBlock->Slot))
			{
				LogTextSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
				LogTextSlot->SetPosition(FVector2D(12.f, 46.f));
				LogTextSlot->SetSize(FVector2D(-12.f, -104.f));
				LogTextSlot->SetAutoSize(false);
				LogTextSlot->SetZOrder(10);
			}
		}
		else if (TextName.Contains(TEXT("SummaryText")))
		{
			ApplyLegacyTextStyle(TextBlock, 10, FLinearColor(0.64f, 0.72f, 0.73f, 1.f));
			if (UCanvasPanelSlot* SummaryTextSlot = Cast<UCanvasPanelSlot>(TextBlock->Slot))
			{
				SummaryTextSlot->SetAnchors(FAnchors(0.f, 1.f, 1.f, 1.f));
				SummaryTextSlot->SetPosition(FVector2D(12.f, -142.f));
				SummaryTextSlot->SetSize(FVector2D(-12.f, 132.f));
				SummaryTextSlot->SetAutoSize(false);
				SummaryTextSlot->SetZOrder(20);
			}
		}
	}

	for (const TWeakObjectPtr<UUserWidget>& WidgetPtr : StyledWidgets)
	{
		UUserWidget* OwnerWidget = WidgetPtr.Get();
		if (!OwnerWidget || !OwnerWidget->WidgetTree)
		{
			continue;
		}

		if (UCanvasPanel* Canvas = Cast<UCanvasPanel>(OwnerWidget->WidgetTree->RootWidget))
		{
			if (!OwnerWidget->WidgetTree->FindWidget(FName(TEXT("CombatLogLegacyBackdrop"))))
			{
				UBorder* Backdrop = OwnerWidget->WidgetTree->ConstructWidget<UBorder>(
					UBorder::StaticClass(),
					FName(TEXT("CombatLogLegacyBackdrop")));
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
		}

		ApplyLegacyButtonStyle(OwnerWidget, ActiveFilter, TEXT("Button_151"), TEXT("全部"), ECombatLogFilter::All);
		ApplyLegacyButtonStyle(OwnerWidget, ActiveFilter, TEXT("Normal"), TEXT("普通"), ECombatLogFilter::Normal);
		ApplyLegacyButtonStyle(OwnerWidget, ActiveFilter, TEXT("Crit"), TEXT("暴击"), ECombatLogFilter::Crit);
		ApplyLegacyButtonStyle(OwnerWidget, ActiveFilter, TEXT("Rune"), TEXT("符文"), ECombatLogFilter::Rune);
		ApplyLegacyButtonStyle(OwnerWidget, ActiveFilter, TEXT("Bleed"), TEXT("流血"), ECombatLogFilter::Bleed);
		RebuildLegacySegmentedLog(OwnerWidget, ActiveFilter);
	}
}
#else
static void ApplyLegacyCombatLogWidgetStyle(ECombatLogFilter) {}
#endif

FString UCombatLogStatics::GetEntryText(const FDamageBreakdown& Entry)
{
	FString Result;
	for (const FCombatLogTextSegment& Segment : GetEntryTextSegments(Entry, false))
	{
		Result += Segment.Text;
	}
	return Result;
}

TArray<FCombatLogTextSegment> UCombatLogStatics::GetEntryTextSegments(const FDamageBreakdown& Entry, bool bDebugMode)
{
	static const FLinearColor TimeColor(0.55f, 0.6f, 0.62f, 1.f);
	static const FLinearColor SourceColor(0.78f, 0.48f, 1.f, 1.f);
	static const FLinearColor TargetColor(0.45f, 0.85f, 1.f, 1.f);
	static const FLinearColor TextColor(0.88f, 0.9f, 0.9f, 1.f);
	static const FLinearColor DamageColor(1.f, 0.25f, 0.25f, 1.f);
	static const FLinearColor HealColor(0.35f, 1.f, 0.45f, 1.f);
	static const FLinearColor DebugColor(0.62f, 0.68f, 0.72f, 1.f);

	TArray<FCombatLogTextSegment> Segments;
	const FString Time = FormatCombatLogTime(Entry.GameTime);
	const FString SourceName = GetDisplayActorName(Entry.SourceName);
	const FString TargetName = GetDisplayActorName(Entry.TargetName);
	const FString TypeStr = Entry.DamageType.ToString();

	AddSegment(Segments, Time, TimeColor);
	AddSegment(Segments, TEXT(" "), TextColor);

	if (Entry.bIsCardEventOnly)
	{
		const FString TimingStr = Entry.CardConsumeTiming.IsNone()
			? TEXT("")
			: FString::Printf(TEXT(" (%s)"), *Entry.CardConsumeTiming.ToString());
		AddSegment(Segments, SourceName, SourceColor);
		AddSegment(Segments, TEXT("使用了"), TextColor);
		AddSegment(Segments, GetCardNameForLog(Entry), GetEntryColor(Entry));
		AddSegment(Segments, TimingStr, DebugColor);
		if (Entry.DamageType == FName(TEXT("Card_Shuffle")))
		{
			AddSegment(Segments, TEXT("，并触发洗牌"), GetEntryColor(Entry));
		}
		AddSegment(Segments, TEXT("。"), TextColor);
	}
	else
	{
		const bool bRune = TypeStr.StartsWith(TEXT("Rune"));
		const bool bBleed = Entry.DamageType == FName(TEXT("Bleed"));
		const bool bCard = IsCardType(TypeStr);
		const FString DamageText = FormatDamageValue(Entry.FinalDamage);

		if (bRune)
		{
			AddSegment(Segments, SourceName, SourceColor);
			AddSegment(Segments, TEXT("施放了"), TextColor);
			AddSegment(Segments, Entry.ActionName.IsNone() ? FString(TEXT("符文")) : Entry.ActionName.ToString(), GetEntryColor(Entry));
			AddSegment(Segments, TEXT("，对"), TextColor);
			AddSegment(Segments, TargetName, TargetColor);
			AddSegment(Segments, TEXT("造成"), TextColor);
		}
		else if (bBleed)
		{
			AddSegment(Segments, TargetName, TargetColor);
			AddSegment(Segments, TEXT("受到了来自"), TextColor);
			AddSegment(Segments, SourceName, SourceColor);
			AddSegment(Segments, TEXT("的流血伤害，损失"), TextColor);
		}
		else if (bCard)
		{
			AddSegment(Segments, SourceName, SourceColor);
			AddSegment(Segments, TEXT("的"), TextColor);
			AddSegment(Segments, GetCardNameForLog(Entry), GetEntryColor(Entry));
			AddSegment(Segments, TEXT("击中了"), TextColor);
			AddSegment(Segments, TargetName, TargetColor);
			AddSegment(Segments, TEXT("，造成"), TextColor);
		}
		else
		{
			AddSegment(Segments, SourceName, SourceColor);
			AddSegment(Segments, TEXT("击中了"), TextColor);
			AddSegment(Segments, TargetName, TargetColor);
			AddSegment(Segments, TEXT("，造成"), TextColor);
		}

		AddSegment(Segments, DamageText, DamageColor);
		AddSegment(Segments, TEXT("点伤害"), TextColor);
		if (Entry.bIsCrit)
		{
			AddSegment(Segments, TEXT(" 暴击"), FLinearColor(1.f, 0.9f, 0.1f, 1.f));
		}
		if (Entry.bHasTargetVitals)
		{
			AddSegment(Segments, TEXT(" ("), TextColor);
			AddSegment(Segments, FString::Printf(TEXT("%s->%s"),
				*FormatDamageValue(Entry.TargetHealthBefore),
				*FormatDamageValue(Entry.TargetHealthAfter)), HealColor);
			AddSegment(Segments, TEXT(")"), TextColor);
		}
		AddSegment(Segments, TEXT("。"), TextColor);
	}

	if (bDebugMode)
	{
		AddSegment(Segments, FString::Printf(TEXT(" [DamageType=%s; Formula=%sx%.2fx%.2f; Flags=%s"),
			*TypeStr,
			*FormatDamageValue(Entry.BaseAttack),
			Entry.ActionMultiplier,
			Entry.DmgTakenMult,
			*BuildDebugFlags(Entry)), DebugColor);
		if (!Entry.CardDisplayName.IsNone())
		{
			AddSegment(Segments, FString::Printf(TEXT("; Card=%s"), *Entry.CardDisplayName.ToString()), DebugColor);
		}
		if (Entry.bHasTargetVitals && Entry.TargetArmorBefore > 0.f)
		{
			AddSegment(Segments, FString::Printf(TEXT("; Armor=%s->%s"),
				*FormatDamageValue(Entry.TargetArmorBefore),
				*FormatDamageValue(Entry.TargetArmorAfter)), DebugColor);
		}
		AddSegment(Segments, TEXT("]"), DebugColor);
	}

	return Segments;
}

FLinearColor UCombatLogStatics::GetEntryColor(const FDamageBreakdown& Entry)
{
	const FString TypeStr = Entry.DamageType.ToString();

	// 优先级：终结技 > 连携 > 匹配 > 普通卡牌/消耗/洗牌 > 暴击 > 符文 > 流血 > 普通
	if (Entry.DamageType == FName("Card_Finisher"))
		return FLinearColor(1.f, 0.85f, 0.f);
	if (Entry.DamageType == FName("Card_Link"))
		return FLinearColor(1.f, 0.55f, 0.1f);
	if (Entry.DamageType == FName("Card_Matched"))
		return FLinearColor(0.2f, 0.9f, 0.9f);
	if (Entry.DamageType == FName("Card_Shuffle"))
		return FLinearColor(0.5f, 0.7f, 1.f);
	if (IsCardType(TypeStr))
		return FLinearColor(0.4f, 0.7f, 1.f);
	if (Entry.DamageType == FName("Bleed"))
		return FLinearColor(1.f, 0.2f, 0.2f);
	if (TypeStr.StartsWith("Rune"))
		return FLinearColor(0.8f, 0.4f, 1.f);
	if (Entry.bIsCrit)
		return FLinearColor(1.f, 0.9f, 0.1f);
	return FLinearColor(1.f, 1.f, 1.f);
}

bool UCombatLogStatics::PassesFilter(const FDamageBreakdown& Entry, ECombatLogFilter Filter)
{
	const FString TypeStr = Entry.DamageType.ToString();

	switch (Filter)
	{
	case ECombatLogFilter::All:      return true;
	case ECombatLogFilter::Normal:   return !Entry.bIsCrit
	                                     && !IsCardType(TypeStr)
	                                     && Entry.DamageType != FName("Bleed")
	                                     && !TypeStr.StartsWith("Rune");
	case ECombatLogFilter::Crit:     return Entry.bIsCrit;
	case ECombatLogFilter::Rune:     return TypeStr.StartsWith("Rune");
	case ECombatLogFilter::Bleed:    return Entry.DamageType == FName("Bleed");
	case ECombatLogFilter::Card:     return IsCardType(TypeStr);
	case ECombatLogFilter::Finisher: return Entry.DamageType == FName("Card_Finisher");
	case ECombatLogFilter::Link:     return Entry.DamageType == FName("Card_Link");
	case ECombatLogFilter::Shuffle:  return Entry.DamageType == FName("Card_Shuffle");
	default:                         return true;
	}
}

bool UCombatLogStatics::PassesAdvancedFilter(
	const FDamageBreakdown& Entry,
	ECombatLogFilter Filter,
	const FString& SourceFilter,
	const FString& TargetFilter,
	float CurrentTime,
	float TimeWindowSeconds)
{
	if (!PassesFilter(Entry, Filter))
	{
		return false;
	}

	if (TimeWindowSeconds > 0.f && CurrentTime > 0.f && Entry.GameTime < CurrentTime - TimeWindowSeconds)
	{
		return false;
	}

	const FString DisplaySource = GetDisplayActorName(Entry.SourceName);
	if (!IsAllFilterText(SourceFilter)
		&& !DisplaySource.Contains(SourceFilter, ESearchCase::IgnoreCase)
		&& !Entry.SourceName.Contains(SourceFilter, ESearchCase::IgnoreCase))
	{
		return false;
	}

	const FString DisplayTarget = GetDisplayActorName(Entry.TargetName);
	if (!IsAllFilterText(TargetFilter)
		&& !DisplayTarget.Contains(TargetFilter, ESearchCase::IgnoreCase)
		&& !Entry.TargetName.Contains(TargetFilter, ESearchCase::IgnoreCase))
	{
		return false;
	}

	return true;
}

FString UCombatLogStatics::GetDisplayActorName(const FString& RawName)
{
	FString Name = RawName;
	Name.TrimStartAndEndInline();
	if (Name.IsEmpty())
	{
		return TEXT("未知目标");
	}

	static const TCHAR* Prefixes[] =
	{
		TEXT("BP_"),
		TEXT("BPC_"),
		TEXT("SK_"),
		TEXT("SM_"),
		TEXT("WBP_"),
	};

	bool bRemovedPrefix = true;
	while (bRemovedPrefix)
	{
		bRemovedPrefix = false;
		for (const TCHAR* Prefix : Prefixes)
		{
			if (Name.StartsWith(Prefix))
			{
				Name.RightChopInline(FCString::Strlen(Prefix));
				bRemovedPrefix = true;
				break;
			}
		}
	}

	int32 LastUnderscore = INDEX_NONE;
	if (Name.FindLastChar(TEXT('_'), LastUnderscore) && LastUnderscore < Name.Len() - 1)
	{
		const FString Tail = Name.RightChop(LastUnderscore + 1);
		if (Tail.IsNumeric())
		{
			Name.LeftInline(LastUnderscore);
		}
	}

	if (Name.EndsWith(TEXT("_C")))
	{
		Name.LeftChopInline(2);
	}

	int32 ClassSuffixIndex = INDEX_NONE;
	if (Name.FindChar(TEXT('_'), ClassSuffixIndex))
	{
		const FString ClassMarker = TEXT("_C_");
		if (Name.Find(ClassMarker, ESearchCase::CaseSensitive) != INDEX_NONE)
		{
			Name.LeftInline(Name.Find(ClassMarker, ESearchCase::CaseSensitive));
		}
	}

	Name.ReplaceInline(TEXT("_"), TEXT(" "));
	Name.TrimStartAndEndInline();
	return Name.IsEmpty() ? FString(TEXT("未知目标")) : Name;
}

FString UCombatLogStatics::GetFormattedLog(ECombatLogFilter Filter)
{
	ApplyLegacyCombatLogWidgetStyle(Filter);

	FString Result;

	for (const FDamageBreakdown& E : Entries)
	{
		if (!PassesFilter(E, Filter)) continue;
		Result += GetEntryText(E) + TEXT("\n");
	}

	if (Result.IsEmpty())
		Result = TEXT("(暂无记录)");

	return Result;
}

FString UCombatLogStatics::GetFormattedSummary()
{
	float Normal = 0, Crit = 0, Rune = 0, Bleed = 0;
	float CardHit = 0, CardLink = 0, CardFinisher = 0;
	int32 HN = 0, HC = 0, HR = 0, HB = 0;
	int32 HCardConsume = 0, HCardHit = 0, HCardMatched = 0;
	int32 HCardLink = 0, HCardFinisher = 0, HCardShuffle = 0;

	for (const FDamageBreakdown& E : Entries)
	{
		const FString T = E.DamageType.ToString();
		if (E.DamageType == FName("Card_Consume") || E.DamageType == FName("Card_Shuffle"))
		{
			HCardConsume++;
			if (E.bStartedShuffle) HCardShuffle++;
		}
		else if (E.DamageType == FName("Card_Finisher"))
		{
			HCardFinisher++;
			HCardHit++;
			CardFinisher += E.FinalDamage;
		}
		else if (E.DamageType == FName("Card_Link"))
		{
			HCardLink++;
			HCardHit++;
			CardLink += E.FinalDamage;
		}
		else if (E.DamageType == FName("Card_Matched"))
		{
			HCardMatched++;
			HCardHit++;
			CardHit += E.FinalDamage;
		}
		else if (E.DamageType == FName("Card_Hit"))
		{
			HCardHit++;
			CardHit += E.FinalDamage;
		}
		else if (T.StartsWith("Rune"))  { Rune  += E.FinalDamage; HR++; }
		else if (E.DamageType == FName("Bleed")) { Bleed += E.FinalDamage; HB++; }
		else if (E.bIsCrit)             { Crit  += E.FinalDamage; HC++; }
		else                            { Normal += E.FinalDamage; HN++; }
	}

	const float Total = Normal + Crit + Rune + Bleed + CardHit + CardLink + CardFinisher;

	return FString::Printf(
		TEXT("-- 卡牌统计 --\n")
		TEXT("消耗: %d次 | 命中: %d次 | 匹配: %d次 | 连携: %d次 | 终结技: %d次 | 洗牌: %d次\n")
		TEXT("-- 伤害统计 --\n")
		TEXT("普通: %.0f(%d次) | 暴击: %.0f(%d次) | 符文: %.0f(%d次) | 流血: %.0f(%d次)\n")
		TEXT("卡牌命中: %.0f(%d次) | 终结技: %.0f(%d次) | 连携: %.0f(%d次)\n")
		TEXT("总计: %.0f"),
		HCardConsume, HCardHit, HCardMatched, HCardLink, HCardFinisher, HCardShuffle,
		Normal, HN, Crit, HC, Rune, HR, Bleed, HB,
		CardHit, HCardHit, CardFinisher, HCardFinisher, CardLink, HCardLink,
		Total);
}
