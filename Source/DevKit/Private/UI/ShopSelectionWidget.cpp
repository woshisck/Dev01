#include "UI/ShopSelectionWidget.h"

#include "Algo/RandomShuffle.h"
#include "Blueprint/WidgetTree.h"
#include "Character/PlayerCharacterBase.h"
#include "Component/BackpackGridComponent.h"
#include "Component/CombatDeckComponent.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Data/RuneDataAsset.h"
#include "Input/CommonUIInputTypes.h"
#include "InputCoreTypes.h"
#include "Map/ShopActor.h"
#include "UI/YogHUD.h"

namespace
{
UTextBlock* MakeText(UWidgetTree* Tree, const FName Name, const FText& Text, int32 FontSize)
{
	UTextBlock* TextBlock = Tree ? Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name) : nullptr;
	if (!TextBlock)
	{
		return nullptr;
	}
	TextBlock->SetText(Text);
	FSlateFontInfo Font = TextBlock->GetFont();
	Font.Size = FontSize;
	TextBlock->SetFont(Font);
	TextBlock->SetAutoWrapText(true);
	return TextBlock;
}

UButton* MakeTextButton(UWidgetTree* Tree, const FName Name, const FText& Text)
{
	UButton* Button = Tree ? Tree->ConstructWidget<UButton>(UButton::StaticClass(), Name) : nullptr;
	if (!Button)
	{
		return nullptr;
	}

	const FName LabelName(*FString::Printf(TEXT("%s_Label"), *Name.ToString()));
	UTextBlock* Label = MakeText(Tree, LabelName, Text, 18);
	if (Label)
	{
		Label->SetJustification(ETextJustify::Center);
		Button->AddChild(Label);
	}
	return Button;
}
}

void UShopSelectionWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BuildFallbackLayout();

	if (ItemButton0) ItemButton0->OnClicked.AddDynamic(this, &UShopSelectionWidget::OnItem0Clicked);
	if (ItemButton1) ItemButton1->OnClicked.AddDynamic(this, &UShopSelectionWidget::OnItem1Clicked);
	if (ItemButton2) ItemButton2->OnClicked.AddDynamic(this, &UShopSelectionWidget::OnItem2Clicked);
	if (ItemButton3) ItemButton3->OnClicked.AddDynamic(this, &UShopSelectionWidget::OnItem3Clicked);
	if (ItemButton4) ItemButton4->OnClicked.AddDynamic(this, &UShopSelectionWidget::OnItem4Clicked);
	if (ItemButton5) ItemButton5->OnClicked.AddDynamic(this, &UShopSelectionWidget::OnItem5Clicked);
	if (BtnClose) BtnClose->OnClicked.AddDynamic(this, &UShopSelectionWidget::OnCloseClicked);

	SetVisibility(ESlateVisibility::Collapsed);
}

TOptional<FUIInputConfig> UShopSelectionWidget::GetDesiredInputConfig() const
{
	return FUIInputConfig(ECommonInputMode::Menu, EMouseCaptureMode::NoCapture);
}

void UShopSelectionWidget::NativeOnActivated()
{
	Super::NativeOnActivated();
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	SetIsFocusable(true);

	if (APlayerController* PC = GetOwningPlayer())
	{
		if (AYogHUD* HUD = Cast<AYogHUD>(PC->GetHUD()))
		{
			HUD->BeginPauseEffect();
		}

		PC->SetShowMouseCursor(true);
		FInputModeUIOnly InputMode;
		InputMode.SetWidgetToFocus(GetCachedWidget());
		PC->SetInputMode(InputMode);
	}
	SetUserFocus(GetOwningPlayer());
}

void UShopSelectionWidget::NativeOnDeactivated()
{
	SetVisibility(ESlateVisibility::Collapsed);
	if (APlayerController* PC = GetOwningPlayer())
	{
		PC->SetShowMouseCursor(false);
		PC->SetInputMode(FInputModeGameOnly());
		if (AYogHUD* HUD = Cast<AYogHUD>(PC->GetHUD()))
		{
			HUD->EndPauseEffect();
		}
	}
	Super::NativeOnDeactivated();
}

FReply UShopSelectionWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	if (Key == EKeys::Escape || Key == EKeys::Gamepad_FaceButton_Right || Key == EKeys::Gamepad_Special_Right)
	{
		CloseShop();
		return FReply::Handled();
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UShopSelectionWidget::Setup(UShopDataAsset* InData, APlayerCharacterBase* InPlayer, AShopActor* InSourceShop)
{
	const bool bNewShopContext = ShopData != InData || SourceShop.Get() != InSourceShop;
	ShopData = InData;
	OwningPlayer = InPlayer;
	SourceShop = InSourceShop;

	if (bNewShopContext || CurrentEntries.IsEmpty())
	{
		PurchasedIndices.Reset();
		CurrentEntries.Reset();

		if (ShopData)
		{
			TArray<FShopRuneEntry> Pool = ShopData->StockPool;
			if (ShopData->bShuffleStockOnOpen)
			{
				Algo::RandomShuffle(Pool);
			}

			const int32 MaxCount = FMath::Clamp(ShopData->MaxVisibleItems, 1, 6);
			for (const FShopRuneEntry& Entry : Pool)
			{
				if (!Entry.RuneAsset)
				{
					continue;
				}
				CurrentEntries.Add(Entry);
				if (CurrentEntries.Num() >= MaxCount)
				{
					break;
				}
			}
		}
	}

	OnShopStockReady(CurrentEntries);
	RefreshNativeView();
}

void UShopSelectionWidget::BuyItem(int32 ItemIndex)
{
	APlayerCharacterBase* Player = OwningPlayer.Get();
	if (!Player || !CurrentEntries.IsValidIndex(ItemIndex))
	{
		return;
	}

	if (PurchasedIndices.Contains(ItemIndex))
	{
		const FText Message = NSLOCTEXT("ShopSelection", "AlreadyPurchased", "\u5df2\u552e\u51fa");
		OnShopPurchaseResult(ItemIndex, false, Message);
		if (StatusText) StatusText->SetText(Message);
		return;
	}

	const FShopRuneEntry& Entry = CurrentEntries[ItemIndex];
	URuneDataAsset* Rune = Entry.RuneAsset;
	if (!Rune)
	{
		return;
	}

	UBackpackGridComponent* Backpack = Player->GetBackpackGridComponent();
	if (!Backpack)
	{
		return;
	}

	const int32 Cost = GetEntryCost(Entry);
	if (!Backpack->SpendGold(Cost))
	{
		const FText Message = FText::Format(
			NSLOCTEXT("ShopSelection", "NotEnoughGold", "\u91d1\u5e01\u4e0d\u8db3\uff1a\u9700\u8981 {0} G"),
			FText::AsNumber(Cost));
		OnShopPurchaseResult(ItemIndex, false, Message);
		if (StatusText) StatusText->SetText(Message);
		RefreshGoldText();
		return;
	}

	Player->AddRuneToInventory(Rune->CreateInstance());
	if (Player->CombatDeckComponent)
	{
		Player->CombatDeckComponent->AddCardFromRuneShop(Rune);
	}

	if (Entry.bSoldOutAfterPurchase)
	{
		PurchasedIndices.Add(ItemIndex);
	}

	const FText Message = FText::Format(
		NSLOCTEXT("ShopSelection", "PurchaseSuccess", "\u5df2\u8d2d\u4e70\uff1a{0}"),
		GetEntryName(Entry));
	OnShopPurchaseResult(ItemIndex, true, Message);
	if (StatusText) StatusText->SetText(Message);
	RefreshNativeView();
}

void UShopSelectionWidget::CloseShop()
{
	DeactivateWidget();
}

void UShopSelectionWidget::BuildFallbackLayout()
{
	if (!WidgetTree || ItemBox)
	{
		return;
	}

	UVerticalBox* Root = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ShopRoot"));
	WidgetTree->RootWidget = Root;

	TitleText = MakeText(WidgetTree, TEXT("TitleText"), NSLOCTEXT("ShopSelection", "Title", "\u5546\u5e97"), 24);
	Root->AddChildToVerticalBox(TitleText);

	GoldText = MakeText(WidgetTree, TEXT("GoldText"), FText::GetEmpty(), 16);
	Root->AddChildToVerticalBox(GoldText);

	StatusText = MakeText(WidgetTree, TEXT("StatusText"), FText::GetEmpty(), 15);
	Root->AddChildToVerticalBox(StatusText);

	ItemBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ItemBox"));
	Root->AddChildToVerticalBox(ItemBox);

	ItemButton0 = MakeTextButton(WidgetTree, TEXT("ItemButton0"), FText::GetEmpty());
	ItemButton1 = MakeTextButton(WidgetTree, TEXT("ItemButton1"), FText::GetEmpty());
	ItemButton2 = MakeTextButton(WidgetTree, TEXT("ItemButton2"), FText::GetEmpty());
	ItemButton3 = MakeTextButton(WidgetTree, TEXT("ItemButton3"), FText::GetEmpty());
	ItemButton4 = MakeTextButton(WidgetTree, TEXT("ItemButton4"), FText::GetEmpty());
	ItemButton5 = MakeTextButton(WidgetTree, TEXT("ItemButton5"), FText::GetEmpty());
	for (UButton* Button : GetItemButtons())
	{
		ItemBox->AddChildToVerticalBox(Button);
	}

	BtnClose = MakeTextButton(WidgetTree, TEXT("BtnClose"), NSLOCTEXT("ShopSelection", "Close", "\u5173\u95ed"));
	Root->AddChildToVerticalBox(BtnClose);
}

void UShopSelectionWidget::RefreshNativeView()
{
	if (TitleText)
	{
		TitleText->SetText(NSLOCTEXT("ShopSelection", "Title", "\u5546\u5e97"));
	}
	RefreshGoldText();
	RefreshItemButtons();
}

void UShopSelectionWidget::RefreshGoldText()
{
	if (!GoldText)
	{
		return;
	}

	int32 Gold = 0;
	if (APlayerCharacterBase* Player = OwningPlayer.Get())
	{
		if (UBackpackGridComponent* Backpack = Player->GetBackpackGridComponent())
		{
			Gold = Backpack->Gold;
		}
	}

	GoldText->SetText(FText::Format(
		NSLOCTEXT("ShopSelection", "GoldFmt", "\u5f53\u524d\u91d1\u5e01\uff1a{0} G"),
		FText::AsNumber(Gold)));
}

void UShopSelectionWidget::RefreshItemButtons()
{
	const TArray<UButton*> Buttons = GetItemButtons();
	for (int32 Index = 0; Index < Buttons.Num(); ++Index)
	{
		UButton* Button = Buttons[Index];
		if (!Button)
		{
			continue;
		}

		const bool bValid = CurrentEntries.IsValidIndex(Index);
		Button->SetVisibility(bValid ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		if (!bValid)
		{
			continue;
		}

		const FShopRuneEntry& Entry = CurrentEntries[Index];
		const FText StateText = PurchasedIndices.Contains(Index)
			? NSLOCTEXT("ShopSelection", "SoldOut", "\u5df2\u552e\u51fa")
			: FText::Format(NSLOCTEXT("ShopSelection", "CostFmt", "{0} G"), FText::AsNumber(GetEntryCost(Entry)));

		SetButtonLabel(Button, FText::Format(
			NSLOCTEXT("ShopSelection", "ItemFmt", "{0}\n{1}"),
			GetEntryName(Entry),
			StateText));
	}
}

int32 UShopSelectionWidget::GetEntryCost(const FShopRuneEntry& Entry) const
{
	if (Entry.OverrideGoldCost >= 0)
	{
		return Entry.OverrideGoldCost;
	}
	return Entry.RuneAsset ? FMath::Max(0, Entry.RuneAsset->RuneInfo.RuneConfig.GoldCost) : 0;
}

FText UShopSelectionWidget::GetEntryName(const FShopRuneEntry& Entry) const
{
	if (!Entry.RuneAsset)
	{
		return NSLOCTEXT("ShopSelection", "MissingRune", "\u672a\u77e5\u7b26\u6587");
	}
	return FText::FromName(Entry.RuneAsset->RuneInfo.RuneConfig.RuneName);
}

void UShopSelectionWidget::SetButtonLabel(UButton* Button, const FText& Text) const
{
	if (!Button)
	{
		return;
	}

	if (UTextBlock* Label = Cast<UTextBlock>(Button->GetChildAt(0)))
	{
		Label->SetText(Text);
	}
}

TArray<UButton*> UShopSelectionWidget::GetItemButtons() const
{
	return { ItemButton0, ItemButton1, ItemButton2, ItemButton3, ItemButton4, ItemButton5 };
}

void UShopSelectionWidget::OnItem0Clicked() { BuyItem(0); }
void UShopSelectionWidget::OnItem1Clicked() { BuyItem(1); }
void UShopSelectionWidget::OnItem2Clicked() { BuyItem(2); }
void UShopSelectionWidget::OnItem3Clicked() { BuyItem(3); }
void UShopSelectionWidget::OnItem4Clicked() { BuyItem(4); }
void UShopSelectionWidget::OnItem5Clicked() { BuyItem(5); }
void UShopSelectionWidget::OnCloseClicked() { CloseShop(); }
