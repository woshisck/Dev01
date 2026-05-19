#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Component/PlayerActiveSkillComponent.h"
#include "ActiveSkillBarWidget.generated.h"

class UBorder;
class UHorizontalBox;
class UImage;
class UProgressBar;
class UTextBlock;

UCLASS()
class DEVKIT_API UActiveSkillBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Active Skill")
	void BindToActiveSkillComponent(UPlayerActiveSkillComponent* InActiveSkillComponent);

	UFUNCTION(BlueprintCallable, Category = "Active Skill")
	void RefreshSkillSlots();

	UFUNCTION(BlueprintPure, Category = "Active Skill")
	UPlayerActiveSkillComponent* GetBoundActiveSkillComponent() const { return BoundActiveSkillComponent; }

	UFUNCTION(BlueprintImplementableEvent, Category = "Active Skill")
	void BP_OnSkillSlotsChanged(const TArray<FActiveSkillSlotView>& Slots);

	UFUNCTION(BlueprintImplementableEvent, Category = "Active Skill")
	void BP_OnSkillUsed(int32 SlotIndex, const FActiveSkillSlotView& SkillSlot);

	UFUNCTION(BlueprintImplementableEvent, Category = "Active Skill")
	void BP_OnSkillUseFailed(int32 SlotIndex, const FText& Reason);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	struct FRuntimeSkillSlotWidget
	{
		TObjectPtr<UBorder> RootBorder = nullptr;
		TObjectPtr<UImage> IconImage = nullptr;
		TObjectPtr<UTextBlock> NameText = nullptr;
		TObjectPtr<UTextBlock> KeyText = nullptr;
		TObjectPtr<UTextBlock> CooldownText = nullptr;
		TObjectPtr<UProgressBar> CooldownBar = nullptr;
	};

	UPROPERTY()
	TObjectPtr<UPlayerActiveSkillComponent> BoundActiveSkillComponent = nullptr;

	UPROPERTY()
	TObjectPtr<UHorizontalBox> RuntimeRoot = nullptr;

	TArray<FRuntimeSkillSlotWidget> RuntimeSlotWidgets;

	void UnbindCurrentComponent();
	void BuildRuntimeLayout();
	void UpdateSlotWidgets(const TArray<FActiveSkillSlotView>& Slots);
	FText GetShortDisplayName(const FActiveSkillSlotView& SkillSlot) const;

	UFUNCTION()
	void HandleSkillSlotsChanged(const TArray<FActiveSkillSlotView>& Slots);

	UFUNCTION()
	void HandleSkillUsed(int32 SlotIndex, FActiveSkillSlotView SkillSlot);

	UFUNCTION()
	void HandleSkillUseFailed(int32 SlotIndex, FText Reason);
};
