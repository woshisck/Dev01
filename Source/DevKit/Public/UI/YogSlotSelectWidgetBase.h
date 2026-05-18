#pragma once

#include "CoreMinimal.h"
#include "SaveGame/YogSaveGame.h"
#include "UI/YogEntryMenuWidget.h"
#include "YogSlotSelectWidgetBase.generated.h"

class UBorder;
class UButton;
class UTextBlock;
class UWidget;
class UYogSaveSubsystem;

UCLASS(Blueprintable)
class DEVKIT_API UYogSlotSelectWidgetBase : public UYogEntryMenuWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Slot Select")
	static FText BuildPreviewSummary(const FSlotPreviewData& Preview);

	UFUNCTION(BlueprintCallable, Category = "Slot Select")
	void RequestAllPreviews();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeOnActivated() override;
	virtual UWidget* NativeGetDesiredFocusTarget() const override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UBorder> SlotCard_0;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SlotTitleText_0;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SlotPreviewText_0;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> BtnContinue_0;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> BtnNewGame_0;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> BtnDelete_0;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UBorder> SlotCard_1;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SlotTitleText_1;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SlotPreviewText_1;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> BtnContinue_1;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> BtnNewGame_1;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> BtnDelete_1;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UBorder> SlotCard_2;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SlotTitleText_2;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SlotPreviewText_2;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> BtnContinue_2;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> BtnNewGame_2;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> BtnDelete_2;

	UFUNCTION()
	void HandleContinueSlot0();

	UFUNCTION()
	void HandleContinueSlot1();

	UFUNCTION()
	void HandleContinueSlot2();

	UFUNCTION()
	void HandleNewGameSlot0();

	UFUNCTION()
	void HandleNewGameSlot1();

	UFUNCTION()
	void HandleNewGameSlot2();

	UFUNCTION()
	void HandleDeleteSlot0();

	UFUNCTION()
	void HandleDeleteSlot1();

	UFUNCTION()
	void HandleDeleteSlot2();

	UFUNCTION()
	void ApplyPreview_0(const FSlotPreviewData& Preview);

	UFUNCTION()
	void ApplyPreview_1(const FSlotPreviewData& Preview);

	UFUNCTION()
	void ApplyPreview_2(const FSlotPreviewData& Preview);

private:
	UYogSaveSubsystem* GetSaveSubsystem() const;
	void BindButtons();
	void SelectSlot(int32 SlotIndex);
	void ContinueSlot(int32 SlotIndex);
	void NewGameSlot(int32 SlotIndex);
	void DeleteSlot(int32 SlotIndex);
	void ApplyPreview(int32 SlotIndex, const FSlotPreviewData& Preview);
	void UpdateSlotWidgets(int32 SlotIndex, const FSlotPreviewData& Preview);

	UTextBlock* GetTitleText(int32 SlotIndex) const;
	UTextBlock* GetPreviewText(int32 SlotIndex) const;
	UButton* GetContinueButton(int32 SlotIndex) const;
	UButton* GetDeleteButton(int32 SlotIndex) const;

	FSlotPreviewData CachedPreviews[3];
};
