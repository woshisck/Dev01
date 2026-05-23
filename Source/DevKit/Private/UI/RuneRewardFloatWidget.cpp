#include "UI/RuneRewardFloatWidget.h"

#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/Widget.h"
#include "Data/RuneDataAsset.h"
#include "Engine/Texture2D.h"
#include "UI/YogCommonRichTextBlock.h"

void URuneRewardFloatWidget::NativeConstruct()
{
	Super::NativeConstruct();
	RefreshPickupHint();
}

void URuneRewardFloatWidget::SetLootOptions(const TArray<FLootOption>& Options)
{
	RefreshPickupHint();

	if (!RuneListBox)
	{
		return;
	}

	RuneListBox->ClearChildren();
	for (const FLootOption& Option : Options)
	{
		if (Option.LootType == ELootType::Rune && Option.RuneAsset)
		{
			const FRuneConfig& Config = Option.RuneAsset->RuneInfo.RuneConfig;
			AddRewardRow(FText::FromName(Config.RuneName), Config.RuneIcon, FLinearColor(0.15f, 0.15f, 0.15f, 1.f));
		}
		else if (Option.LootType == ELootType::Gold)
		{
			UTexture2D* GoldIcon = Option.Icon
				? Option.Icon.Get()
				: Cast<UTexture2D>(StaticLoadObject(
					UTexture2D::StaticClass(),
					nullptr,
					TEXT("/Game/UI/Playtest_UI/UI_Tex/HUD/T_GoldCoinIcon.T_GoldCoinIcon")));
			const FText Label = Option.DisplayName.IsEmpty()
				? FText::Format(NSLOCTEXT("RuneRewardFloat", "GoldRewardFmt", "Gold x{0}"), FText::AsNumber(Option.Amount))
				: Option.DisplayName;
			AddRewardRow(Label, GoldIcon, FLinearColor(0.95f, 0.72f, 0.22f, 1.f));
		}
		else if (Option.LootType == ELootType::Material)
		{
			const FText Label = Option.DisplayName.IsEmpty()
				? FText::Format(NSLOCTEXT("RuneRewardFloat", "MaterialRewardFmt", "Material x{0}"), FText::AsNumber(Option.Amount))
				: Option.DisplayName;
			AddRewardRow(Label, Option.Icon.Get(), FLinearColor(0.35f, 0.32f, 0.42f, 1.f));
		}
	}
}

void URuneRewardFloatWidget::AddRewardRow(const FText& Name, UTexture2D* IconTexture, const FLinearColor& FallbackColor)
{
	if (!RuneListBox)
	{
		return;
	}

	UHorizontalBox* Row = NewObject<UHorizontalBox>(this);

	USizeBox* IconBox = NewObject<USizeBox>(this);
	IconBox->SetWidthOverride(40.f);
	IconBox->SetHeightOverride(40.f);

	UImage* Icon = NewObject<UImage>(this);
	if (IconTexture)
	{
		Icon->SetBrushFromTexture(IconTexture, true);
	}
	else
	{
		Icon->SetColorAndOpacity(FallbackColor);
	}
	IconBox->AddChild(Icon);

	UHorizontalBoxSlot* IconSlot = Row->AddChildToHorizontalBox(IconBox);
	IconSlot->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f));
	IconSlot->SetVerticalAlignment(VAlign_Center);
	IconSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));

	UTextBlock* NameText = NewObject<UTextBlock>(this);
	NameText->SetText(Name);
	UHorizontalBoxSlot* TextSlot = Row->AddChildToHorizontalBox(NameText);
	TextSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	TextSlot->SetVerticalAlignment(VAlign_Center);

	UVerticalBoxSlot* RowSlot = RuneListBox->AddChildToVerticalBox(Row);
	RowSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 6.f));
}

void URuneRewardFloatWidget::RefreshPickupHint()
{
	if (!PickupHintText)
	{
		return;
	}

	if (UYogCommonRichTextBlock* RichHint = Cast<UYogCommonRichTextBlock>(PickupHintText))
	{
		RichHint->SetText(FText::FromString(TEXT("<input action=\"Interact\"/> 拾取")));
		return;
	}

	if (UTextBlock* PlainHint = Cast<UTextBlock>(PickupHintText))
	{
		PlainHint->SetText(NSLOCTEXT("RuneRewardFloat", "PickupHintConfirm", "确认拾取"));
	}
}
