#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PlayerCommonInfoWidget.generated.h"

class UBackpackGridComponent;
class UHorizontalBox;
class UImage;
class UTextBlock;
class UTexture2D;
class UVerticalBox;

UCLASS()
class DEVKIT_API UPlayerCommonInfoWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Player Common Info")
	void BindToBackpack(UBackpackGridComponent* InBackpack);

	UFUNCTION(BlueprintCallable, Category = "Player Common Info")
	void SetGold(int32 Gold);

	UFUNCTION(BlueprintCallable, Category = "Player Common Info")
	void SetCommonInfoEntry(FName EntryId, FText Label, int32 Count, UTexture2D* IconTexture);

	UFUNCTION(BlueprintCallable, Category = "Player Common Info")
	void RemoveCommonInfoEntry(FName EntryId);

	UFUNCTION(BlueprintCallable, Category = "Player Common Info")
	void ClearCommonInfoEntries();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> CommonInfoList;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UHorizontalBox> GoldRow;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UImage> GoldIcon;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> GoldText;

private:
	UFUNCTION()
	void HandleGoldChanged(int32 NewGold);

	void UnbindBackpack();
	void ApplyGoldIconBrush();
	void ConfigureIcon(UImage* Icon, UTexture2D* Texture, const FVector2D& Size) const;
	void ConfigureCountText(UTextBlock* TextBlock, const FText& Text) const;
	UHorizontalBox* CreateCommonInfoEntryRow(FName EntryId, UImage*& OutIcon, UTextBlock*& OutText);
	UTexture2D* LoadGoldIconTexture() const;
	static FName MakeEntryWidgetName(FName EntryId, const TCHAR* Suffix);

	UPROPERTY()
	TObjectPtr<UBackpackGridComponent> BoundBackpack;

	UPROPERTY(Transient)
	TMap<FName, TObjectPtr<UHorizontalBox>> DynamicRows;

	UPROPERTY(Transient)
	TMap<FName, TObjectPtr<UImage>> DynamicIcons;

	UPROPERTY(Transient)
	TMap<FName, TObjectPtr<UTextBlock>> DynamicTexts;
};
