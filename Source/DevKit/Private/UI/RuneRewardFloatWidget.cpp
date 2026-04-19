#include "UI/RuneRewardFloatWidget.h"
#include "Data/RuneDataAsset.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/SizeBox.h"

void URuneRewardFloatWidget::SetLootOptions(const TArray<FLootOption>& Options)
{
	if (!RuneListBox) return;
	RuneListBox->ClearChildren();

	for (const FLootOption& Option : Options)
	{
		if (Option.LootType != ELootType::Rune || !Option.RuneAsset) continue;
		const FRuneConfig& Cfg = Option.RuneAsset->RuneInfo.RuneConfig;

		UHorizontalBox* Row = NewObject<UHorizontalBox>(this);

		// 图标 40×40
		USizeBox* IconBox = NewObject<USizeBox>(this);
		IconBox->SetWidthOverride(40.f);
		IconBox->SetHeightOverride(40.f);
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

		// 名称文字
		UTextBlock* NameTB = NewObject<UTextBlock>(this);
		NameTB->SetText(FText::FromName(Cfg.RuneName));
		UHorizontalBoxSlot* TextSlot = Row->AddChildToHorizontalBox(NameTB);
		TextSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		TextSlot->SetVerticalAlignment(VAlign_Center);

		UVerticalBoxSlot* RowSlot = RuneListBox->AddChildToVerticalBox(Row);
		RowSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 6.f));
	}
}
