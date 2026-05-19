#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "MetaProgression/MetaTypes.h"
#include "YogMetaUpgradeTreeWidgetBase.generated.h"

class UButton;
class UTextBlock;
class UScrollBox;
class UYogMetaProgressionSubsystem;
class UMetaNodeCardWidgetBase;
class UVerticalBox;

UCLASS(Blueprintable)
class DEVKIT_API UYogMetaUpgradeTreeWidgetBase : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	UYogMetaUpgradeTreeWidgetBase(const FObjectInitializer& ObjectInitializer);

	// 刷新所有节点卡片（切换侧别过滤或重新激活时调用）
	UFUNCTION(BlueprintCallable, Category = "MetaUpgrade")
	void RefreshTree();

	// 尝试购买节点，返回是否成功
	UFUNCTION(BlueprintCallable, Category = "MetaUpgrade")
	bool PurchaseNode(FName NodeRowName);

	// 当前显示哪一侧（true=血肉侧，false=神秘侧）
	UPROPERTY(BlueprintReadOnly, Category = "MetaUpgrade")
	bool bShowFleshSide = true;

	UFUNCTION(BlueprintCallable, Category = "MetaUpgrade")
	void SetShowFleshSide(bool bFlesh);

protected:

	virtual void NativeConstruct() override;
	virtual void NativeOnActivated() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MetaUpgrade")
	TSubclassOf<UMetaNodeCardWidgetBase> NodeCardWidgetClass;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UScrollBox> NodeList;

	// BP 实现：用 NodeRowName 创建并添加一张节点卡片到对应容器
	UFUNCTION(BlueprintImplementableEvent, Category = "MetaUpgrade")
	void BP_AddNodeCard(FName NodeRowName, const FMetaUpgradeNodeRow& NodeData,
	                    int32 CurrentLevel, bool bCanPurchase);

	// BP 实现：清空节点容器
	UFUNCTION(BlueprintImplementableEvent, Category = "MetaUpgrade")
	void BP_ClearNodeCards();

	// ── BindWidgetOptional ────────────────────────────────────
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TxtCurrencyAmount;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> BtnFleshSide;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> BtnMysticSide;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> BtnClose;

private:

	UFUNCTION()
	void HandleNodePurchased(FName NodeRowName);

	UFUNCTION()
	void HandleCurrencyChanged(FGameplayTag CurrencyTag, int32 NewAmount);

	UFUNCTION()
	void HandleFleshSideClicked();

	UFUNCTION()
	void HandleMysticSideClicked();

	UFUNCTION()
	void HandleCloseClicked();

	UYogMetaProgressionSubsystem* GetMetaSys() const;
	bool NativeAddNodeCard(FName NodeRowName, const FMetaUpgradeNodeRow& NodeData,
	                       int32 CurrentLevel, bool bCanPurchase);

	UFUNCTION()
	void HandleCardPurchaseRequested(FName NodeRowName);
};
