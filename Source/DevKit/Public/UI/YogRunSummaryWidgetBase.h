#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "MetaProgression/YogMetaProgressionSubsystem.h"
#include "YogRunSummaryWidgetBase.generated.h"

class UButton;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRunSummaryAction);

UCLASS(Blueprintable)
class DEVKIT_API UYogRunSummaryWidgetBase : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:

	// 由 GameMode 死亡/通关时调用，填充并激活结算页
	UFUNCTION(BlueprintCallable, Category = "RunSummary")
	void ShowSummary(const FRunSummaryData& InSummary);

	// "返回主城"按钮点击后广播
	UPROPERTY(BlueprintAssignable, Category = "RunSummary")
	FOnRunSummaryAction OnReturnToHubRequested;

protected:

	virtual void NativeConstruct() override;

	// BP 实现：用 Summary 数据填充 UI 控件（楼层、击杀、货币列表、可解锁节点提示）
	UFUNCTION(BlueprintImplementableEvent, Category = "RunSummary")
	void BP_OnSummaryReceived(const FRunSummaryData& Summary);

	UPROPERTY(BlueprintReadOnly, Category = "RunSummary")
	FRunSummaryData CurrentSummary;

	// ── BindWidgetOptional ────────────────────────────────────
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TxtFloorReached;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TxtEnemiesKilled;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> BtnReturnToHub;

private:

	UFUNCTION()
	void HandleReturnToHubClicked();
};
