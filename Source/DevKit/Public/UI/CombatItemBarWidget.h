#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Component/CombatItemComponent.h"
#include "CombatItemBarWidget.generated.h"

class UBorder;
class UHorizontalBox;
class UImage;
class UProgressBar;
class UTextBlock;

UCLASS()
class DEVKIT_API UCombatItemBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Combat Item")
	void BindToCombatItemComponent(UCombatItemComponent* InCombatItemComponent);

	UFUNCTION(BlueprintCallable, Category = "Combat Item")
	void RefreshItemSlots();

	UFUNCTION(BlueprintPure, Category = "Combat Item")
	UCombatItemComponent* GetBoundCombatItemComponent() const { return BoundCombatItemComponent; }

	UFUNCTION(BlueprintImplementableEvent, Category = "Combat Item")
	void BP_OnItemSlotsChanged(const TArray<FCombatItemSlotView>& Slots);

	UFUNCTION(BlueprintImplementableEvent, Category = "Combat Item")
	void BP_OnItemUsed(int32 SlotIndex, const FCombatItemSlotView& ItemSlot);

	UFUNCTION(BlueprintImplementableEvent, Category = "Combat Item")
	void BP_OnItemUseFailed(int32 SlotIndex, const FText& Reason);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual void NativeDestruct() override;

private:
	struct FRuntimeSlotWidget
	{
		TObjectPtr<UBorder> RootBorder = nullptr;
		TObjectPtr<UImage> IconImage = nullptr;
		TObjectPtr<UTextBlock> NameText = nullptr;
		TObjectPtr<UTextBlock> CountText = nullptr;
		TObjectPtr<UTextBlock> CooldownText = nullptr;
		TObjectPtr<UProgressBar> CooldownBar = nullptr;
	};

	UPROPERTY()
	TObjectPtr<UCombatItemComponent> BoundCombatItemComponent = nullptr;

	UPROPERTY()
	TObjectPtr<UHorizontalBox> RuntimeRoot = nullptr;

	TArray<FRuntimeSlotWidget> RuntimeSlotWidgets;
	TArray<bool> RuntimeSlotHasEntry;

	void UnbindCurrentComponent();
	void BuildRuntimeLayout();
	void UpdateSlotWidgets(const TArray<FCombatItemSlotView>& Slots);
	void StartSelectionRoll(int32 PreviousIndex, int32 NewIndex);
	void TickSelectionRoll(float DeltaTime);
	void ApplySelectionPresentation(float Alpha = 1.f);
	void ApplySlotPresentation(int32 SlotIndex, float Opacity, float Scale, float TranslationY);
	FText GetShortDisplayName(const FCombatItemSlotView& ItemSlot) const;

	int32 LastSelectedSlotIndex = INDEX_NONE;
	int32 RollPreviousSlotIndex = INDEX_NONE;
	int32 RollNewSlotIndex = INDEX_NONE;
	float RollTimer = 0.f;
	bool bSelectionRollActive = false;

	static constexpr float RollDurationSeconds = 0.32f;

	UFUNCTION()
	void HandleItemSlotsChanged(const TArray<FCombatItemSlotView>& Slots);

	UFUNCTION()
	void HandleItemUsed(int32 SlotIndex, FCombatItemSlotView ItemSlot);

	UFUNCTION()
	void HandleItemUseFailed(int32 SlotIndex, FText Reason);
};
