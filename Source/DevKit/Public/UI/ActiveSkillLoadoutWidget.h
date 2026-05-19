#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "Component/PlayerActiveSkillComponent.h"
#include "ActiveSkillLoadoutWidget.generated.h"

class UActiveSkillDataAsset;
class UButton;
class UPlayerActiveSkillComponent;
class UTextBlock;
class UVerticalBox;

UCLASS()
class DEVKIT_API UActiveSkillLoadoutWidget : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	UActiveSkillLoadoutWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Active Skill")
	TArray<TObjectPtr<UActiveSkillDataAsset>> AvailableSkillAssets;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Active Skill")
	TSoftObjectPtr<UActiveSkillDataAsset> DefaultSkillAsset;

	UFUNCTION(BlueprintCallable, Category = "Active Skill")
	void RefreshLoadout();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeOnActivated() override;
	virtual void NativeOnDeactivated() override;

private:
	UPROPERTY()
	TObjectPtr<UVerticalBox> RuntimeRoot = nullptr;

	UPROPERTY()
	TObjectPtr<UTextBlock> StatusText = nullptr;

	UPROPERTY()
	TObjectPtr<UButton> Slot0Button = nullptr;

	UPROPERTY()
	TObjectPtr<UButton> Slot1Button = nullptr;

	UPROPERTY()
	TObjectPtr<UTextBlock> Slot0Text = nullptr;

	UPROPERTY()
	TObjectPtr<UTextBlock> Slot1Text = nullptr;

	UPROPERTY()
	TObjectPtr<UButton> CloseButton = nullptr;

	void BuildRuntimeLayout();
	UPlayerActiveSkillComponent* ResolveActiveSkillComponent() const;
	UActiveSkillDataAsset* ResolveDefaultSkill() const;
	void AssignDefaultSkillToSlot(int32 SlotIndex);
	void PersistLoadout(UPlayerActiveSkillComponent* ActiveSkillComponent);
	FText DescribeSlot(const TArray<FActiveSkillSlotView>& Slots, int32 SlotIndex) const;

	UFUNCTION()
	void HandleSlot0Clicked();

	UFUNCTION()
	void HandleSlot1Clicked();

	UFUNCTION()
	void HandleCloseClicked();
};
