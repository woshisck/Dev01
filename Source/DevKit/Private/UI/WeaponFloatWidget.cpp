#include "UI/WeaponFloatWidget.h"
#include "UI/YogCommonRichTextBlock.h"
#include "Item/Weapon/WeaponDefinition.h"
#include "Item/Weapon/WeaponInfoDA.h"
#include "Data/RuneDataAsset.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Border.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/PanelWidget.h"
#include "Components/ScrollBox.h"
#include "Components/ScrollBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/Widget.h"
#include "CommonRichTextBlock.h"
#include "Input/Reply.h"
#include "InputCoreTypes.h"

namespace WeaponZoneColors
{
	static const FLinearColor Active  (0.20f, 0.80f, 1.00f, 1.0f);
	static const FLinearColor Inactive(0.15f, 0.15f, 0.18f, 0.7f);

	FText GetCombatCardDisplayName(const URuneDataAsset* RuneDA)
	{
		if (!RuneDA)
		{
			return FText::GetEmpty();
		}

		const FCombatCardConfig& Card = RuneDA->RuneInfo.CombatCard;
		if (!Card.DisplayName.IsEmpty())
		{
			return Card.DisplayName;
		}

		return FText::FromName(RuneDA->GetRuneName());
	}

	FString NormalizeCardSummaryText(FString Text)
	{
		Text.ReplaceInline(TEXT("\r"), TEXT(" "));
		Text.ReplaceInline(TEXT("\n"), TEXT(" "));
		Text.ReplaceInline(TEXT("\t"), TEXT(" "));
		while (Text.ReplaceInline(TEXT("  "), TEXT(" ")) > 0)
		{
		}
		Text.TrimStartAndEndInline();
		return Text;
	}

	bool IsCardSummaryNoiseSentence(const FString& Text)
	{
		return Text.IsEmpty()
			|| Text.Equals(TEXT("近战武器卡"))
			|| Text.Equals(TEXT("远程武器卡"))
			|| Text.Equals(TEXT("普通卡牌"))
			|| Text.Equals(TEXT("任意攻击"))
			|| Text.Contains(TEXT("测试"))
			|| Text.Contains(TEXT("效果 Tag"))
			|| Text.Contains(TEXT("默认正向连携"))
			|| Text.Contains(TEXT("读取上一张"));
	}

	void SplitSummarySentences(const FString& Text, TArray<FString>& OutSentences)
	{
		FString Current;
		for (const TCHAR Ch : Text)
		{
			const bool bDelimiter = Ch == TEXT('。')
				|| Ch == TEXT('；')
				|| Ch == TEXT(';')
				|| Ch == TEXT('.')
				|| Ch == TEXT('!')
				|| Ch == TEXT('！')
				|| Ch == TEXT('?')
				|| Ch == TEXT('？');
			if (bDelimiter)
			{
				Current.TrimStartAndEndInline();
				if (!Current.IsEmpty())
				{
					OutSentences.Add(Current);
				}
				Current.Reset();
			}
			else
			{
				Current.AppendChar(Ch);
			}
		}

		Current.TrimStartAndEndInline();
		if (!Current.IsEmpty())
		{
			OutSentences.Add(Current);
		}
	}

	FString ShortenCardSummary(FString Summary, int32 MaxCharacters)
	{
		Summary = NormalizeCardSummaryText(Summary);
		Summary.ReplaceInline(TEXT("玩家攻击时生成一道"), TEXT("攻击生成"));
		Summary.ReplaceInline(TEXT("玩家攻击时"), TEXT("攻击时"));
		Summary.ReplaceInline(TEXT("攻击命中时，对"), TEXT("命中："));
		Summary.ReplaceInline(TEXT("攻击命中时，"), TEXT("命中："));
		Summary.ReplaceInline(TEXT("攻击命中时,"), TEXT("命中："));
		Summary.ReplaceInline(TEXT("对受伤敌人周围"), TEXT("周围"));
		Summary.ReplaceInline(TEXT(" 内其他敌人造成本次攻击 "), TEXT("内敌人受"));
		Summary.ReplaceInline(TEXT("内其他敌人造成本次攻击"), TEXT("内敌人受"));
		Summary.ReplaceInline(TEXT(" 的溅射伤害"), TEXT("溅射伤害"));
		Summary.ReplaceInline(TEXT("的溅射伤害"), TEXT("溅射伤害"));
		Summary.ReplaceInline(TEXT(" 300cm "), TEXT("300cm"));
		Summary.ReplaceInline(TEXT(" "), TEXT(""));
		Summary.TrimStartAndEndInline();

		MaxCharacters = FMath::Max(MaxCharacters, 8);
		if (Summary.Len() > MaxCharacters)
		{
			Summary = Summary.Left(MaxCharacters - 1) + TEXT("…");
		}
		return Summary;
	}

	FText GetCombatCardSummary(const URuneDataAsset* RuneDA, int32 MaxCharacters)
	{
		if (!RuneDA)
		{
			return FText::GetEmpty();
		}

		const FCombatCardConfig& Card = RuneDA->RuneInfo.CombatCard;
		if (!Card.HUDSummaryText.IsEmpty())
		{
			return Card.HUDSummaryText;
		}

		FString Source = Card.HUDReasonText.IsEmpty()
			? RuneDA->GetRuneDescription().ToString()
			: Card.HUDReasonText.ToString();
		Source = NormalizeCardSummaryText(Source);

		TArray<FString> Sentences;
		SplitSummarySentences(Source, Sentences);
		for (FString Sentence : Sentences)
		{
			Sentence = NormalizeCardSummaryText(Sentence);
			if (!IsCardSummaryNoiseSentence(Sentence))
			{
				const FString Summary = ShortenCardSummary(Sentence, MaxCharacters);
				return Summary.IsEmpty() ? FText::GetEmpty() : FText::FromString(Summary);
			}
		}

		const FString Summary = ShortenCardSummary(Source, MaxCharacters);
		return Summary.IsEmpty() ? FText::GetEmpty() : FText::FromString(Summary);
	}

	void ConfigureTextBlock(UTextBlock* TextBlock, const FSlateColor& Color, int32 FontSize, bool bWrap)
	{
		if (!TextBlock)
		{
			return;
		}

		FSlateFontInfo Font = TextBlock->GetFont();
		Font.Size = FontSize;
		TextBlock->SetFont(Font);
		TextBlock->SetColorAndOpacity(Color);
		TextBlock->SetAutoWrapText(bWrap);
		if (bWrap)
		{
			TextBlock->SetWrapTextAt(250.f);
		}
	}

	void ConfigureCardScrollBox(UScrollBox* ScrollBox, float WheelStep)
	{
		if (!ScrollBox)
		{
			return;
		}

		ScrollBox->SetVisibility(ESlateVisibility::Visible);
		ScrollBox->SetOrientation(Orient_Vertical);
		ScrollBox->SetScrollBarVisibility(ESlateVisibility::Collapsed);
		ScrollBox->SetConsumeMouseWheel(EConsumeMouseWheel::Always);
		ScrollBox->SetWheelScrollMultiplier(FMath::Max(WheelStep, 1.f));
		ScrollBox->SetAllowOverscroll(false);
		ScrollBox->SetAnimateWheelScrolling(false);
		ScrollBox->SetClipping(EWidgetClipping::ClipToBoundsAlways);
	}

	void SetRichHint(UCommonRichTextBlock* TextBlock, const FText& Text)
	{
		if (!TextBlock)
		{
			return;
		}

		TextBlock->SetText(Text);
		TextBlock->SetVisibility(Text.IsEmpty()
			? ESlateVisibility::Collapsed
			: ESlateVisibility::SelfHitTestInvisible);
	}
}

void UWeaponFloatWidget::NativeConstruct()
{
	Super::NativeConstruct();

	SetIsFocusable(true);
	EnsureRuntimeCardScrollBox();
}

void UWeaponFloatWidget::SetWeaponDefinition(const UWeaponDefinition* Def)
{
	if (!Def) return;

	EnsureRuntimeCardScrollBox();
	SetRenderTransform(FWidgetTransform());
	SetRenderOpacity(1.f);
	// 折叠动画把 InfoContainer 透明度降到 0、Visibility=Collapsed，必须两个都还原
	bCollapsing   = false;
	CollapseTimer = 0.f;
	if (InfoContainer)
	{
		InfoContainer->SetRenderOpacity(1.f);
		InfoContainer->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}

	const UWeaponInfoDA* Info = Def->WeaponInfo;

	// ── 缩略图 ─────────────────────────────────
	CachedThumbnail = nullptr;
	if (WeaponThumbnail)
	{
		if (Info && Info->Thumbnail)
		{
			WeaponThumbnail->SetBrushFromTexture(Info->Thumbnail, true);
			WeaponThumbnail->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			CachedThumbnail = Info->Thumbnail;
		}
		else
		{
			WeaponThumbnail->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	// ── 名称 ───────────────────────────────────
	if (WeaponNameText)
		WeaponNameText->SetText(Info ? Info->WeaponName : FText::GetEmpty());

	// ── 描述 ───────────────────────────────────
	if (WeaponDescText)
	{
		const bool bHas = Info && !Info->WeaponDescription.IsEmpty();
		WeaponDescText->SetVisibility(bHas ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
		if (bHas)
		{
			WeaponDescText->SetText(Info->WeaponDescription);
			WeaponDescText->SetAutoWrapText(true);
		}
	}

	// ── 子描述 ─────────────────────────────────
	if (WeaponSubDescText)
	{
		const bool bHas = Info && !Info->WeaponSubDescription.IsEmpty();
		WeaponSubDescText->SetVisibility(bHas ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
		if (bHas)
		{
			WeaponSubDescText->SetText(Info->WeaponSubDescription);
			WeaponSubDescText->SetAutoWrapText(true);
		}
	}

	// ── 激活区 ─────────────────────────────────
	const FActivationZoneConfig& ZoneCfg = Def->BackpackConfig.ActivationZoneConfig;
	const int32 GW = Def->BackpackConfig.GridWidth;
	const int32 GH = Def->BackpackConfig.GridHeight;

	auto GetShape   = [&](int32 Idx) -> const FRuneShape*
	{
		return ZoneCfg.ZoneShapes.IsValidIndex(Idx) ? &ZoneCfg.ZoneShapes[Idx] : nullptr;
	};
	auto GetZoneImg = [&](int32 Idx) -> UTexture2D*
	{
		if (!Info) return nullptr;
		switch (Idx) {
			case 0: return Info->Zone1Image;
			case 1: return Info->Zone2Image;
			case 2: return Info->Zone3Image;
		}
		return nullptr;
	};

	BuildZonePanel(ZoneGrid1, Zone1Image, GetZoneImg(0), GetShape(0), GW, GH);
	BuildZonePanel(ZoneGrid2, Zone2Image, GetZoneImg(1), GetShape(1), GW, GH);
	BuildZonePanel(ZoneGrid3, Zone3Image, GetZoneImg(2), GetShape(2), GW, GH);

	// ── 512 初始战斗卡牌列表 ───────────────────
	const TArray<TObjectPtr<URuneDataAsset>>& SourceCards = Def->InitialCombatDeck.IsEmpty()
		? Def->InitialRunes
		: Def->InitialCombatDeck;
	BuildCombatCardList(SourceCards);
	RefreshOperationHints();
}

// ─────────────────────────────────────────────────────────────────────────────
//  激活区：图像覆盖 or 全格点阵
// ─────────────────────────────────────────────────────────────────────────────

void UWeaponFloatWidget::BuildZonePanel(UCanvasPanel* GridPanel, UImage* ImgWidget,
                                         UTexture2D* ZoneTexture, const FRuneShape* Shape,
                                         int32 GW, int32 GH)
{
	if (ZoneTexture && ImgWidget)
	{
		ImgWidget->SetBrushFromTexture(ZoneTexture, true);
		ImgWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		if (GridPanel) GridPanel->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	if (ImgWidget) ImgWidget->SetVisibility(ESlateVisibility::Collapsed);
	if (!GridPanel) return;

	GridPanel->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	GridPanel->ClearChildren();

	TSet<FIntPoint> ActiveCells;
	if (Shape) ActiveCells.Append(Shape->Cells);

	const int32 MaxDim  = FMath::Max(FMath::Max(GW, GH), 1);
	const float Step    = ZoneGridSize / MaxDim;
	const float DotSize = FMath::Max(Step - 2.f, 1.f);
	const float OffX    = (ZoneGridSize - (GW * Step - (Step - DotSize))) * 0.5f;
	const float OffY    = (ZoneGridSize - (GH * Step - (Step - DotSize))) * 0.5f;
	const float Radius  = FMath::Max(DotSize * 0.25f, 1.f);

	for (int32 Row = 0; Row < GH; Row++)
	{
		for (int32 Col = 0; Col < GW; Col++)
		{
			const bool bActive = ActiveCells.Contains(FIntPoint(Col, Row));

			UImage* Dot = NewObject<UImage>(this);
			FSlateBrush Brush;
			Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
			Brush.TintColor = FSlateColor(FLinearColor::White);
			Brush.OutlineSettings.CornerRadii  = FVector4(Radius, Radius, Radius, Radius);
			Brush.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
			Dot->SetBrush(Brush);
			Dot->SetColorAndOpacity(bActive ? WeaponZoneColors::Active : WeaponZoneColors::Inactive);

			UCanvasPanelSlot* DotSlot = GridPanel->AddChildToCanvas(Dot);
			DotSlot->SetPosition(FVector2D(OffX + Col * Step, OffY + Row * Step));
			DotSlot->SetSize(FVector2D(DotSize, DotSize));
		}
	}
}

// ─────────────────────────────────────────────────────────────────────────────
//  初始卡牌列表
// ─────────────────────────────────────────────────────────────────────────────

void UWeaponFloatWidget::BuildCombatCardList(const TArray<TObjectPtr<URuneDataAsset>>& Cards)
{
	if (!RuneListBox) return;
	EnsureRuntimeCardScrollBox();
	RuneListBox->ClearChildren();

	UTextBlock* Header = NewObject<UTextBlock>(this);
	Header->SetText(NSLOCTEXT("WeaponFloatWidget", "InitialCombatDeckHeader", "初始卡牌"));
	WeaponZoneColors::ConfigureTextBlock(Header, FSlateColor(FLinearColor(0.90f, 0.82f, 0.56f, 1.f)), 15, false);
	if (UVerticalBoxSlot* HeaderSlot = RuneListBox->AddChildToVerticalBox(Header))
	{
		HeaderSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
	}

	int32 AddedCardCount = 0;
	for (const TObjectPtr<URuneDataAsset>& RuneDA : Cards)
	{
		if (!RuneDA) continue;
		const FCombatCardConfig& Card = RuneDA->RuneInfo.CombatCard;
		if (!Card.bIsCombatCard)
		{
			continue;
		}

		const FRuneConfig& Cfg = RuneDA->RuneInfo.RuneConfig;

		UHorizontalBox* Row = NewObject<UHorizontalBox>(this);

		USizeBox* IconBox = NewObject<USizeBox>(this);
		IconBox->SetWidthOverride(30.f);
		IconBox->SetHeightOverride(30.f);
		UImage* Icon = NewObject<UImage>(this);
		if (Cfg.RuneIcon)
			Icon->SetBrushFromTexture(Cfg.RuneIcon, true);
		else
			Icon->SetColorAndOpacity(FLinearColor(0.15f, 0.15f, 0.15f, 1.f));
		IconBox->AddChild(Icon);

		UHorizontalBoxSlot* IconSlot = Row->AddChildToHorizontalBox(IconBox);
		IconSlot->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f));
		IconSlot->SetVerticalAlignment(VAlign_Center);
		IconSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));

		UVerticalBox* TextCol = NewObject<UVerticalBox>(this);
		UTextBlock* NameTB = NewObject<UTextBlock>(this);
		NameTB->SetText(WeaponZoneColors::GetCombatCardDisplayName(RuneDA));
		WeaponZoneColors::ConfigureTextBlock(NameTB, FSlateColor(FLinearColor(0.96f, 0.96f, 0.98f, 1.f)), 16, true);
		TextCol->AddChildToVerticalBox(NameTB);

		const FText Summary = WeaponZoneColors::GetCombatCardSummary(RuneDA, 38);
		if (!Summary.IsEmpty())
		{
			UTextBlock* DescTB = NewObject<UTextBlock>(this);
			DescTB->SetText(Summary);
			WeaponZoneColors::ConfigureTextBlock(DescTB, FSlateColor(FLinearColor(0.80f, 0.84f, 0.88f, 1.f)), 11, true);
			TextCol->AddChildToVerticalBox(DescTB);
		}

		UHorizontalBoxSlot* TextSlot = Row->AddChildToHorizontalBox(TextCol);
		TextSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		TextSlot->SetVerticalAlignment(VAlign_Center);

		UVerticalBoxSlot* RowSlot = RuneListBox->AddChildToVerticalBox(Row);
		RowSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
		AddedCardCount++;
	}

	if (AddedCardCount == 0)
	{
		UTextBlock* EmptyText = NewObject<UTextBlock>(this);
		EmptyText->SetText(NSLOCTEXT("WeaponFloatWidget", "NoInitialCombatDeck", "未配置初始卡牌"));
		WeaponZoneColors::ConfigureTextBlock(EmptyText, FSlateColor(FLinearColor(0.72f, 0.74f, 0.78f, 1.f)), 14, true);
		if (UVerticalBoxSlot* EmptySlot = RuneListBox->AddChildToVerticalBox(EmptyText))
		{
			EmptySlot->SetPadding(FMargin(0.f, 2.f, 0.f, 0.f));
		}
	}

	if (CardScrollBox)
	{
		CardScrollBox->SetScrollOffset(0.f);
		CardScrollBox->InvalidateLayoutAndVolatility();
	}
}

void UWeaponFloatWidget::EnsureRuntimeCardScrollBox()
{
	if (!RuneListBox)
	{
		return;
	}

	if (CardScrollBox)
	{
		WeaponZoneColors::ConfigureCardScrollBox(CardScrollBox, CardScrollStep);
		if (RuneListBox->GetParent() != CardScrollBox)
		{
			if (UPanelWidget* ExistingParent = RuneListBox->GetParent())
			{
				ExistingParent->RemoveChild(RuneListBox);
			}
			CardScrollBox->AddChild(RuneListBox);
		}
		return;
	}

	UPanelWidget* Parent = RuneListBox->GetParent();
	if (!Parent)
	{
		return;
	}

	Parent->RemoveChild(RuneListBox);
	UScrollBox* RuntimeScrollBox = NewObject<UScrollBox>(this, TEXT("RuntimeCardScrollBox"));
	WeaponZoneColors::ConfigureCardScrollBox(RuntimeScrollBox, CardScrollStep);
	RuntimeScrollBox->AddChild(RuneListBox);
	CardScrollBox = RuntimeScrollBox;

	if (UVerticalBox* ParentVBox = Cast<UVerticalBox>(Parent))
	{
		if (UVerticalBoxSlot* VerticalSlot = ParentVBox->AddChildToVerticalBox(RuntimeScrollBox))
		{
			VerticalSlot->SetHorizontalAlignment(HAlign_Fill);
			VerticalSlot->SetVerticalAlignment(VAlign_Fill);
			VerticalSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		}
		return;
	}

	if (UOverlay* ParentOverlay = Cast<UOverlay>(Parent))
	{
		if (UOverlaySlot* OverlaySlot = ParentOverlay->AddChildToOverlay(RuntimeScrollBox))
		{
			OverlaySlot->SetHorizontalAlignment(HAlign_Fill);
			OverlaySlot->SetVerticalAlignment(VAlign_Fill);
		}
		return;
	}

	if (UCanvasPanel* ParentCanvas = Cast<UCanvasPanel>(Parent))
	{
		if (UCanvasPanelSlot* CanvasSlot = ParentCanvas->AddChildToCanvas(RuntimeScrollBox))
		{
			CanvasSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
			CanvasSlot->SetOffsets(FMargin(0.f));
		}
		return;
	}

	if (UBorder* ParentBorder = Cast<UBorder>(Parent))
	{
		ParentBorder->SetContent(RuntimeScrollBox);
		return;
	}

	Parent->AddChild(RuntimeScrollBox);
}

bool UWeaponFloatWidget::ScrollCardList(float Direction)
{
	if (!CardScrollBox || FMath::IsNearlyZero(Direction))
	{
		return false;
	}

	const float CurrentOffset = CardScrollBox->GetScrollOffset();
	const float MaxOffset = FMath::Max(0.f, CardScrollBox->GetScrollOffsetOfEnd());
	const float TargetOffset = CurrentOffset + Direction * CardScrollStep;
	const float NewOffset = MaxOffset > 0.f
		? FMath::Clamp(TargetOffset, 0.f, MaxOffset)
		: FMath::Max(0.f, TargetOffset);
	if (FMath::IsNearlyEqual(NewOffset, CurrentOffset, 0.01f))
	{
		return false;
	}

	CardScrollBox->SetScrollOffset(NewOffset);
	CardScrollBox->InvalidateLayoutAndVolatility();
	return true;
}

FReply UWeaponFloatWidget::NativeOnMouseWheel(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (ScrollCardList(-InMouseEvent.GetWheelDelta()))
	{
		return FReply::Handled();
	}

	return Super::NativeOnMouseWheel(InGeometry, InMouseEvent);
}

FReply UWeaponFloatWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	if (Key == EKeys::Up || Key == EKeys::Gamepad_DPad_Up || Key == EKeys::Gamepad_LeftStick_Up)
	{
		return ScrollCardList(-1.f) ? FReply::Handled() : Super::NativeOnKeyDown(InGeometry, InKeyEvent);
	}
	if (Key == EKeys::Down || Key == EKeys::Gamepad_DPad_Down || Key == EKeys::Gamepad_LeftStick_Down)
	{
		return ScrollCardList(1.f) ? FReply::Handled() : Super::NativeOnKeyDown(InGeometry, InKeyEvent);
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UWeaponFloatWidget::RefreshOperationHints()
{
	const FText PickupHint = NSLOCTEXT(
		"WeaponFloatWidget",
		"PickupAndBackpackHint",
		"按 <input action=\"Interact\"/> 拾取武器后按 <input action=\"OpenBackpack\"/> 打开背包");
	const FText ScrollHint = NSLOCTEXT(
		"WeaponFloatWidget",
		"ScrollCardHint",
		"滚轮 / ↑↓ / 十字键上下 查看卡牌");

	if (ScrollHintText)
	{
		WeaponZoneColors::SetRichHint(PickupHintText, PickupHint);
		WeaponZoneColors::SetRichHint(ScrollHintText, ScrollHint);
	}
	else
	{
		const FText CombinedHint = FText::Format(
			NSLOCTEXT("WeaponFloatWidget", "CombinedWeaponFloatHint", "{0}\n{1}"),
			PickupHint,
			ScrollHint);
		WeaponZoneColors::SetRichHint(PickupHintText, CombinedHint);
	}
}

// ─────────────────────────────────────────────────────────────────────────────
//  折叠动画
// ─────────────────────────────────────────────────────────────────────────────

void UWeaponFloatWidget::StartCollapse(float InDuration)
{
	CollapseDuration = FMath::Max(InDuration, 0.05f);
	CollapseTimer    = 0.f;
	bCollapsing      = true;
}

void UWeaponFloatWidget::BroadcastCollapseComplete(FVector2D ThumbnailScreenCenter)
{
	FOnWeaponFloatCollapseComplete CompletionCallback = OnCollapseComplete;
	OnCollapseComplete.Unbind();

	if (CompletionCallback.IsBound())
	{
		CompletionCallback.Execute(ThumbnailScreenCenter);
	}
}

void UWeaponFloatWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!bCollapsing) return;

	CollapseTimer += InDeltaTime;
	const float Alpha = FMath::Clamp(CollapseTimer / CollapseDuration, 0.f, 1.f);

	// InfoContainer 淡出（点阵 + 符文列表），WeaponThumbnail 保持不变
	if (InfoContainer)
		InfoContainer->SetRenderOpacity(1.f - Alpha);

	if (Alpha >= 1.f)
	{
		bCollapsing = false;
		if (InfoContainer)
			InfoContainer->SetVisibility(ESlateVisibility::Collapsed);

		// 获取缩略图的屏幕绝对中心坐标作为飞行起点
		FVector2D ThumbnailCenter = FVector2D::ZeroVector;
		if (WeaponThumbnail)
		{
			const FGeometry& G = WeaponThumbnail->GetCachedGeometry();
			ThumbnailCenter = G.LocalToAbsolute(G.GetLocalSize() * 0.5f);
		}

		BroadcastCollapseComplete(ThumbnailCenter);
	}
}
