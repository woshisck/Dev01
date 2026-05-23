#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayTagContainer.h"
#include "PlayerCommonInfoWidget.generated.h"

class UBackpackGridComponent;
class UHorizontalBox;
class UImage;
class UYogMetaProgressionSubsystem;
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
	void SetMaterial(int32 Material);

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

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UHorizontalBox> MaterialRow;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UImage> MaterialIcon;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> MaterialText;

private:
	UFUNCTION()
	void HandleGoldChanged(int32 NewGold);

	UFUNCTION()
	void HandleMetaCurrencyChanged(FGameplayTag CurrencyTag, int32 NewAmount);

	void UnbindBackpack();
	void BindToMetaProgression();
	void UnbindMetaProgression();
	void ApplyGoldIconBrush();
	void ApplyMaterialIconBrush();
	FGameplayTag GetPrimaryMaterialCurrencyTag() const;
	void ConfigureIcon(UImage* Icon, UTexture2D* Texture, const FVector2D& Size) const;
	void ConfigureCountText(UTextBlock* TextBlock, const FText& Text) const;
	UHorizontalBox* CreateCommonInfoEntryRow(FName EntryId, UImage*& OutIcon, UTextBlock*& OutText);
	UTexture2D* LoadGoldIconTexture() const;
	UTexture2D* LoadMaterialIconTexture() const;
	static FName MakeEntryWidgetName(FName EntryId, const TCHAR* Suffix);

	UPROPERTY()
	TObjectPtr<UBackpackGridComponent> BoundBackpack;

	UPROPERTY()
	TObjectPtr<UYogMetaProgressionSubsystem> BoundMetaProgression;

	UPROPERTY(Transient)
	TMap<FName, TObjectPtr<UHorizontalBox>> DynamicRows;

	UPROPERTY(Transient)
	TMap<FName, TObjectPtr<UImage>> DynamicIcons;

	UPROPERTY(Transient)
	TMap<FName, TObjectPtr<UTextBlock>> DynamicTexts;
};
