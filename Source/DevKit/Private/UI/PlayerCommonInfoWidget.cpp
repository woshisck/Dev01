#include "UI/PlayerCommonInfoWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Component/BackpackGridComponent.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Texture2D.h"

void UPlayerCommonInfoWidget::NativeConstruct()
{
	Super::NativeConstruct();

	ApplyGoldIconBrush();
	SetGold(BoundBackpack ? BoundBackpack->Gold : 0);
	SetVisibility(ESlateVisibility::HitTestInvisible);
	if (GoldRow)
	{
		GoldRow->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
}

void UPlayerCommonInfoWidget::NativeDestruct()
{
	UnbindBackpack();
	ClearCommonInfoEntries();

	Super::NativeDestruct();
}

void UPlayerCommonInfoWidget::BindToBackpack(UBackpackGridComponent* InBackpack)
{
	if (BoundBackpack == InBackpack)
	{
		SetGold(BoundBackpack ? BoundBackpack->Gold : 0);
		return;
	}

	UnbindBackpack();
	BoundBackpack = InBackpack;

	if (BoundBackpack)
	{
		BoundBackpack->OnGoldChanged.RemoveDynamic(this, &UPlayerCommonInfoWidget::HandleGoldChanged);
		BoundBackpack->OnGoldChanged.AddDynamic(this, &UPlayerCommonInfoWidget::HandleGoldChanged);
		SetGold(BoundBackpack->Gold);
	}
	else
	{
		SetGold(0);
	}
}

void UPlayerCommonInfoWidget::SetGold(int32 Gold)
{
	if (GoldText)
	{
		ConfigureCountText(GoldText, FText::AsNumber(FMath::Max(0, Gold)));
	}
}

void UPlayerCommonInfoWidget::SetCommonInfoEntry(FName EntryId, FText Label, int32 Count, UTexture2D* IconTexture)
{
	if (EntryId.IsNone())
	{
		return;
	}

	if (EntryId == TEXT("Gold"))
	{
		SetGold(Count);
		return;
	}

	if (Count <= 0)
	{
		RemoveCommonInfoEntry(EntryId);
		return;
	}

	UHorizontalBox* Row = DynamicRows.FindRef(EntryId);
	UImage* Icon = DynamicIcons.FindRef(EntryId);
	UTextBlock* Text = DynamicTexts.FindRef(EntryId);
	if (!Row || !Icon || !Text)
	{
		Row = CreateCommonInfoEntryRow(EntryId, Icon, Text);
	}

	if (!Row || !Text)
	{
		return;
	}

	const FText CountText = FText::AsNumber(Count);
	ConfigureCountText(
		Text,
		Label.IsEmpty()
			? CountText
			: FText::Format(NSLOCTEXT("PlayerCommonInfoWidget", "EntryLabelCount", "{0} {1}"), Label, CountText));

	if (Icon)
	{
		ConfigureIcon(Icon, IconTexture, FVector2D(22.0f, 22.0f));
	}

	Row->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UPlayerCommonInfoWidget::RemoveCommonInfoEntry(FName EntryId)
{
	if (TObjectPtr<UHorizontalBox>* Row = DynamicRows.Find(EntryId))
	{
		if (Row->Get())
		{
			Row->Get()->RemoveFromParent();
		}
	}

	DynamicRows.Remove(EntryId);
	DynamicIcons.Remove(EntryId);
	DynamicTexts.Remove(EntryId);
}

void UPlayerCommonInfoWidget::ClearCommonInfoEntries()
{
	TArray<FName> EntryIds;
	DynamicRows.GenerateKeyArray(EntryIds);
	for (const FName& EntryId : EntryIds)
	{
		RemoveCommonInfoEntry(EntryId);
	}
}

void UPlayerCommonInfoWidget::HandleGoldChanged(int32 NewGold)
{
	SetGold(NewGold);
}

void UPlayerCommonInfoWidget::UnbindBackpack()
{
	if (BoundBackpack)
	{
		BoundBackpack->OnGoldChanged.RemoveDynamic(this, &UPlayerCommonInfoWidget::HandleGoldChanged);
		BoundBackpack = nullptr;
	}
}

void UPlayerCommonInfoWidget::ApplyGoldIconBrush()
{
	ConfigureIcon(GoldIcon, LoadGoldIconTexture(), FVector2D(24.0f, 24.0f));
}

void UPlayerCommonInfoWidget::ConfigureIcon(UImage* Icon, UTexture2D* Texture, const FVector2D& Size) const
{
	if (!Icon)
	{
		return;
	}

	if (Texture)
	{
		Icon->SetBrushFromTexture(Texture, false);
		Icon->SetDesiredSizeOverride(Size);
		Icon->SetColorAndOpacity(FLinearColor::White);
		Icon->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	else
	{
		Icon->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UPlayerCommonInfoWidget::ConfigureCountText(UTextBlock* TextBlock, const FText& Text) const
{
	if (!TextBlock)
	{
		return;
	}

	TextBlock->SetText(Text);
	TextBlock->SetColorAndOpacity(FSlateColor(FLinearColor(0.95f, 0.83f, 0.42f, 1.0f)));
	TextBlock->SetShadowOffset(FVector2D(1.0f, 1.0f));
	TextBlock->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.8f));

	FSlateFontInfo FontInfo = TextBlock->GetFont();
	FontInfo.Size = 18;
	TextBlock->SetFont(FontInfo);
}

UHorizontalBox* UPlayerCommonInfoWidget::CreateCommonInfoEntryRow(FName EntryId, UImage*& OutIcon, UTextBlock*& OutText)
{
	OutIcon = nullptr;
	OutText = nullptr;

	if (!CommonInfoList || !WidgetTree)
	{
		return nullptr;
	}

	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(
		UHorizontalBox::StaticClass(),
		MakeUniqueObjectName(WidgetTree, UHorizontalBox::StaticClass(), MakeEntryWidgetName(EntryId, TEXT("Row"))));
	USizeBox* IconBox = WidgetTree->ConstructWidget<USizeBox>(
		USizeBox::StaticClass(),
		MakeUniqueObjectName(WidgetTree, USizeBox::StaticClass(), MakeEntryWidgetName(EntryId, TEXT("IconBox"))));
	OutIcon = WidgetTree->ConstructWidget<UImage>(
		UImage::StaticClass(),
		MakeUniqueObjectName(WidgetTree, UImage::StaticClass(), MakeEntryWidgetName(EntryId, TEXT("Icon"))));
	OutText = WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(),
		MakeUniqueObjectName(WidgetTree, UTextBlock::StaticClass(), MakeEntryWidgetName(EntryId, TEXT("Text"))));

	if (!Row || !IconBox || !OutIcon || !OutText)
	{
		return nullptr;
	}

	Row->SetVisibility(ESlateVisibility::HitTestInvisible);
	IconBox->SetWidthOverride(22.0f);
	IconBox->SetHeightOverride(22.0f);
	IconBox->AddChild(OutIcon);

	if (UHorizontalBoxSlot* IconSlot = Row->AddChildToHorizontalBox(IconBox))
	{
		IconSlot->SetVerticalAlignment(VAlign_Center);
		IconSlot->SetPadding(FMargin(0.0f, 0.0f, 6.0f, 0.0f));
	}

	if (UHorizontalBoxSlot* TextSlot = Row->AddChildToHorizontalBox(OutText))
	{
		TextSlot->SetVerticalAlignment(VAlign_Center);
	}

	if (UVerticalBoxSlot* RowSlot = CommonInfoList->AddChildToVerticalBox(Row))
	{
		RowSlot->SetHorizontalAlignment(HAlign_Right);
		RowSlot->SetPadding(FMargin(0.0f, 2.0f));
	}

	DynamicRows.Add(EntryId, Row);
	DynamicIcons.Add(EntryId, OutIcon);
	DynamicTexts.Add(EntryId, OutText);
	return Row;
}

UTexture2D* UPlayerCommonInfoWidget::LoadGoldIconTexture() const
{
	return Cast<UTexture2D>(StaticLoadObject(
		UTexture2D::StaticClass(),
		nullptr,
		TEXT("/Game/UI/Playtest_UI/UI_Tex/HUD/T_GoldCoinIcon.T_GoldCoinIcon")));
}

FName UPlayerCommonInfoWidget::MakeEntryWidgetName(FName EntryId, const TCHAR* Suffix)
{
	FString CleanId = EntryId.ToString();
	for (TCHAR& Character : CleanId)
	{
		if (!FChar::IsAlnum(Character))
		{
			Character = TCHAR('_');
		}
	}

	return FName(*FString::Printf(TEXT("CommonInfo_%s_%s"), *CleanId, Suffix));
}
