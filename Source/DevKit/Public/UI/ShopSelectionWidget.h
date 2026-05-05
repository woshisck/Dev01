#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "Data/ShopDataAsset.h"
#include "ShopSelectionWidget.generated.h"

class APlayerCharacterBase;
class AShopActor;
class UButton;
class UTextBlock;
class UVerticalBox;

UCLASS(Blueprintable)
class DEVKIT_API UShopSelectionWidget : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	void Setup(UShopDataAsset* InData, APlayerCharacterBase* InPlayer, AShopActor* InSourceShop = nullptr);

	UFUNCTION(BlueprintCallable, Category = "Shop")
	void BuyItem(int32 ItemIndex);

	UFUNCTION(BlueprintCallable, Category = "Shop")
	void CloseShop();

	UFUNCTION(BlueprintImplementableEvent, Category = "Shop")
	void OnShopStockReady(const TArray<FShopRuneEntry>& Entries);

	UFUNCTION(BlueprintImplementableEvent, Category = "Shop")
	void OnShopPurchaseResult(int32 ItemIndex, bool bSuccess, const FText& Message);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeOnActivated() override;
	virtual void NativeOnDeactivated() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TitleText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> GoldText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> StatusText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> ItemBox;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> ItemButton0;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> ItemButton1;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> ItemButton2;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> ItemButton3;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> ItemButton4;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> ItemButton5;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> BtnClose;

	TWeakObjectPtr<APlayerCharacterBase> OwningPlayer;
	TWeakObjectPtr<AShopActor> SourceShop;
	TObjectPtr<UShopDataAsset> ShopData;
	TArray<FShopRuneEntry> CurrentEntries;
	TSet<int32> PurchasedIndices;

private:
	void BuildFallbackLayout();
	void RefreshNativeView();
	void RefreshGoldText();
	void RefreshItemButtons();
	int32 GetEntryCost(const FShopRuneEntry& Entry) const;
	FText GetEntryName(const FShopRuneEntry& Entry) const;
	void SetButtonLabel(UButton* Button, const FText& Text) const;
	TArray<UButton*> GetItemButtons() const;

	UFUNCTION()
	void OnItem0Clicked();
	UFUNCTION()
	void OnItem1Clicked();
	UFUNCTION()
	void OnItem2Clicked();
	UFUNCTION()
	void OnItem3Clicked();
	UFUNCTION()
	void OnItem4Clicked();
	UFUNCTION()
	void OnItem5Clicked();
	UFUNCTION()
	void OnCloseClicked();
};
