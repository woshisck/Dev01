#include "UI/CurrentRoomBuffWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Data/GenericRuneEffectDA.h"
#include "Data/RoomDataAsset.h"
#include "Data/RuneDataAsset.h"
#include "RuneHudTextUtils.h"

namespace
{
constexpr int32 TitleFontSize = 14;
constexpr int32 RoomNameFontSize = 12;
constexpr int32 BuffNameFontSize = 13;
constexpr int32 BuffDescFontSize = 11;
constexpr int32 BuffEffectFontSize = 11;
constexpr int32 BuffSummaryMaxChars = 34;

const FLinearColor PanelColor(0.015f, 0.017f, 0.020f, 0.78f);
const FLinearColor TitleColor(0.96f, 0.93f, 0.82f, 1.0f);
const FLinearColor RoomNameColor(0.70f, 0.72f, 0.76f, 1.0f);
const FLinearColor BuffNameColor(0.93f, 0.93f, 0.93f, 1.0f);
const FLinearColor BuffDescColor(0.78f, 0.78f, 0.80f, 1.0f);
const FLinearColor BuffEffectColor(0.65f, 0.65f, 0.70f, 1.0f);
const FLinearColor EmptyColor(0.62f, 0.64f, 0.68f, 1.0f);

void SetTextSize(UTextBlock* TextBlock, int32 Size)
{
	if (!TextBlock) return;
	FSlateFontInfo Font = TextBlock->GetFont();
	Font.Size = Size;
	TextBlock->SetFont(Font);
}

FText ResolveRuneDisplayName(const URuneDataAsset& RuneDA)
{
	const FName RuneName = RuneDA.RuneInfo.RuneConfig.RuneName;
	if (!RuneName.IsNone())
	{
		return FText::FromName(RuneName);
	}

#if !UE_BUILD_SHIPPING
	UE_LOG(LogTemp, Warning,
		TEXT("CurrentRoomBuffWidget: RuneDA '%s' has empty RuneConfig.RuneName; showing asset name."),
		*RuneDA.GetName());
	return FText::FromString(RuneDA.GetName());
#else
	UE_LOG(LogTemp, Warning, TEXT("CurrentRoomBuffWidget: RuneDA has empty RuneConfig.RuneName."));
	return NSLOCTEXT("CurrentRoomBuff", "UnnamedRune", "未命名符文");
#endif
}

FText ResolveRoomDisplayName(const URoomDataAsset* RoomData)
{
	if (!RoomData)
	{
		return FText::GetEmpty();
	}

	return RoomData->DisplayName.IsEmptyOrWhitespace()
		? FText::FromName(RoomData->RoomName)
		: RoomData->DisplayName;
}

FText FormatGenericEffectLine(const UGenericRuneEffectDA& Effect)
{
	const bool bHasName = !Effect.DisplayName.IsEmptyOrWhitespace();
	const bool bHasDesc = !Effect.Description.IsEmptyOrWhitespace();
	if (bHasName && bHasDesc)
	{
		return FText::Format(
			NSLOCTEXT("CurrentRoomBuff", "EffectNameDesc", "- {0}: {1}"),
			Effect.DisplayName,
			Effect.Description);
	}
	if (bHasName)
	{
		return FText::Format(
			NSLOCTEXT("CurrentRoomBuff", "EffectNameOnly", "- {0}"),
			Effect.DisplayName);
	}
	if (bHasDesc)
	{
		return FText::Format(
			NSLOCTEXT("CurrentRoomBuff", "EffectDescOnly", "- {0}"),
			Effect.Description);
	}
	return FText::GetEmpty();
}
}

void UCurrentRoomBuffWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!BuffListBox)
	{
		BuildDefaultLayout();
	}

	if (TitleText)
	{
		TitleText->SetText(PanelTitle);
	}
	if (EmptyText)
	{
		EmptyText->SetText(EmptyTextValue);
	}

	SetVisibility(ESlateVisibility::Collapsed);
}

void UCurrentRoomBuffWidget::ShowRoomBuffs(URoomDataAsset* RoomData, const TArray<FBuffEntry>& Buffs)
{
	if (!BuffListBox)
	{
		BuildDefaultLayout();
	}

	if (TitleText)
	{
		TitleText->SetText(PanelTitle);
	}
	if (RoomNameText)
	{
		const FText RoomDisplayName = ResolveRoomDisplayName(RoomData);
		RoomNameText->SetText(RoomDisplayName);
		RoomNameText->SetVisibility(RoomDisplayName.IsEmptyOrWhitespace()
			? ESlateVisibility::Collapsed
			: ESlateVisibility::HitTestInvisible);
	}

	int32 ValidBuffCount = 0;
	for (const FBuffEntry& Buff : Buffs)
	{
		if (Buff.RuneDA)
		{
			ValidBuffCount++;
		}
	}

	RebuildBuffList(Buffs);

	if (EmptyText)
	{
		EmptyText->SetText(EmptyTextValue);
		EmptyText->SetVisibility(ValidBuffCount > 0
			? ESlateVisibility::Collapsed
			: ESlateVisibility::HitTestInvisible);
	}

	const bool bShouldShow = ValidBuffCount > 0 || !bCollapseWhenEmpty;
	SetVisibility(bShouldShow ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
}

void UCurrentRoomBuffWidget::HideRoomBuffs()
{
	if (BuffListBox)
	{
		BuffListBox->ClearChildren();
	}
	SetVisibility(ESlateVisibility::Collapsed);
}

void UCurrentRoomBuffWidget::BuildDefaultLayout()
{
	if (bDefaultLayoutBuilt || !WidgetTree)
	{
		return;
	}

	UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RoomBuffRootCanvas"));
	WidgetTree->RootWidget = RootCanvas;

	USizeBox* PanelSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("PanelSizeBox"));
	PanelSizeBox->SetWidthOverride(DefaultPanelWidth);
	UCanvasPanelSlot* PanelSlot = RootCanvas->AddChildToCanvas(PanelSizeBox);
	PanelSlot->SetAutoSize(true);
	PanelSlot->SetAnchors(FAnchors(0.f, 0.f));
	PanelSlot->SetAlignment(FVector2D(0.f, 0.f));
	PanelSlot->SetPosition(FVector2D::ZeroVector);

	OuterBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("OuterBorder"));
	OuterBorder->SetBrushColor(PanelColor);
	OuterBorder->SetPadding(FMargin(12.f, 10.f));
	PanelSizeBox->AddChild(OuterBorder);

	UVerticalBox* ContentBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ContentBox"));
	OuterBorder->SetContent(ContentBox);

	TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TitleText"));
	TitleText->SetText(PanelTitle);
	TitleText->SetColorAndOpacity(FSlateColor(TitleColor));
	SetTextSize(TitleText, TitleFontSize);
	ContentBox->AddChildToVerticalBox(TitleText);

	RoomNameText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("RoomNameText"));
	RoomNameText->SetColorAndOpacity(FSlateColor(RoomNameColor));
	RoomNameText->SetAutoWrapText(true);
	SetTextSize(RoomNameText, RoomNameFontSize);
	UVerticalBoxSlot* RoomSlot = ContentBox->AddChildToVerticalBox(RoomNameText);
	RoomSlot->SetPadding(FMargin(0.f, 2.f, 0.f, 8.f));

	BuffListBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("BuffListBox"));
	ContentBox->AddChildToVerticalBox(BuffListBox);

	EmptyText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("EmptyText"));
	EmptyText->SetText(EmptyTextValue);
	EmptyText->SetColorAndOpacity(FSlateColor(EmptyColor));
	EmptyText->SetAutoWrapText(true);
	SetTextSize(EmptyText, BuffDescFontSize);
	ContentBox->AddChildToVerticalBox(EmptyText);

	bDefaultLayoutBuilt = true;
}

void UCurrentRoomBuffWidget::RebuildBuffList(const TArray<FBuffEntry>& Buffs)
{
	if (!BuffListBox || !WidgetTree)
	{
		return;
	}

	BuffListBox->ClearChildren();

	for (const FBuffEntry& Entry : Buffs)
	{
		if (!Entry.RuneDA) continue;

		const FRuneConfig& Config = Entry.RuneDA->RuneInfo.RuneConfig;

		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());

		USizeBox* IconBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		IconBox->SetWidthOverride(BuffIconSize);
		IconBox->SetHeightOverride(BuffIconSize);

		UImage* Icon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
		if (Config.RuneIcon)
		{
			Icon->SetBrushFromTexture(Config.RuneIcon, true);
		}
		else
		{
			Icon->SetColorAndOpacity(FLinearColor(0.15f, 0.15f, 0.15f, 1.f));
		}
		IconBox->AddChild(Icon);

		UHorizontalBoxSlot* IconSlot = Row->AddChildToHorizontalBox(IconBox);
		IconSlot->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f));
		IconSlot->SetVerticalAlignment(VAlign_Top);
		IconSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));

		UVerticalBox* TextBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());

		UTextBlock* NameText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		NameText->SetText(ResolveRuneDisplayName(*Entry.RuneDA));
		NameText->SetColorAndOpacity(FSlateColor(BuffNameColor));
		NameText->SetAutoWrapText(true);
		SetTextSize(NameText, BuffNameFontSize);
		TextBox->AddChildToVerticalBox(NameText);

		const FText SummaryText = RuneHudTextUtils::GetRuneHudSummary(Config, BuffSummaryMaxChars);
		if (!SummaryText.IsEmptyOrWhitespace())
		{
			UTextBlock* DescText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
			DescText->SetText(SummaryText);
			DescText->SetColorAndOpacity(FSlateColor(BuffDescColor));
			DescText->SetAutoWrapText(true);
			SetTextSize(DescText, BuffDescFontSize);
			UVerticalBoxSlot* DescSlot = TextBox->AddChildToVerticalBox(DescText);
			DescSlot->SetPadding(FMargin(0.f, 2.f, 0.f, 0.f));
		}

		for (const TObjectPtr<UGenericRuneEffectDA>& EffectPtr : Config.GenericEffects)
		{
			const UGenericRuneEffectDA* Effect = EffectPtr.Get();
			if (!Effect) continue;

			const FText Line = FormatGenericEffectLine(*Effect);
			if (Line.IsEmptyOrWhitespace()) continue;

			UTextBlock* EffectText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
			EffectText->SetText(Line);
			EffectText->SetColorAndOpacity(FSlateColor(BuffEffectColor));
			EffectText->SetAutoWrapText(true);
			SetTextSize(EffectText, BuffEffectFontSize);
			UVerticalBoxSlot* EffectSlot = TextBox->AddChildToVerticalBox(EffectText);
			EffectSlot->SetPadding(FMargin(0.f, 2.f, 0.f, 0.f));
		}

		UHorizontalBoxSlot* TextSlot = Row->AddChildToHorizontalBox(TextBox);
		TextSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		TextSlot->SetVerticalAlignment(VAlign_Center);

		UVerticalBoxSlot* RowSlot = BuffListBox->AddChildToVerticalBox(Row);
		RowSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
	}
}
