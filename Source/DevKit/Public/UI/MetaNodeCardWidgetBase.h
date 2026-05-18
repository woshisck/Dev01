#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MetaProgression/MetaTypes.h"
#include "MetaNodeCardWidgetBase.generated.h"

class UButton;
class UTextBlock;
class UProgressBar;

UENUM(BlueprintType)
enum class EMetaNodeCardState : uint8
{
	Locked      UMETA(DisplayName = "前置未满足"),
	Available   UMETA(DisplayName = "可购买"),
	Purchased   UMETA(DisplayName = "已满级"),
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnNodeCardPurchaseRequested, FName, NodeRowName);

UCLASS(Blueprintable)
class DEVKIT_API UMetaNodeCardWidgetBase : public UUserWidget
{
	GENERATED_BODY()

public:

	// 由 WBP_MetaUpgradeTree 在 BP_AddNodeCard 中调用，传入节点数据
	UFUNCTION(BlueprintCallable, Category = "MetaNodeCard")
	void InitCard(FName InRowName, const FMetaUpgradeNodeRow& InData,
	              int32 InCurrentLevel, bool bInCanPurchase);

	// 刷新显示状态（购买后由树刷新触发）
	UFUNCTION(BlueprintCallable, Category = "MetaNodeCard")
	void RefreshState(int32 InCurrentLevel, bool bInCanPurchase);

	UPROPERTY(BlueprintAssignable, Category = "MetaNodeCard")
	FOnNodeCardPurchaseRequested OnPurchaseRequested;

	UPROPERTY(BlueprintReadOnly, Category = "MetaNodeCard")
	FName RowName;

	UPROPERTY(BlueprintReadOnly, Category = "MetaNodeCard")
	FMetaUpgradeNodeRow NodeData;

	UPROPERTY(BlueprintReadOnly, Category = "MetaNodeCard")
	EMetaNodeCardState CardState = EMetaNodeCardState::Locked;

protected:

	virtual void NativeConstruct() override;

	// BP 实现：根据 CardState 更新视觉（颜色/透明度/按钮交互）
	UFUNCTION(BlueprintImplementableEvent, Category = "MetaNodeCard")
	void BP_OnStateChanged(EMetaNodeCardState NewState);

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TxtNodeName;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TxtLevelProgress;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TxtCost;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> ProgressLevel;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> BtnPurchase;

private:

	UFUNCTION()
	void HandlePurchaseClicked();

	void UpdateVisuals(int32 CurrentLevel, bool bCanPurchase);
};
