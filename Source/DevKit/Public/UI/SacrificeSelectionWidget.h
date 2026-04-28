#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "Data/AltarDataAsset.h"
#include "SacrificeSelectionWidget.generated.h"

class APlayerCharacterBase;

UCLASS(Abstract, Blueprintable)
class DEVKIT_API USacrificeSelectionWidget : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	// 打开前调用，随机抽取三个献祭选项并展示 Phase 0
	void Setup(UAltarDataAsset* InData, APlayerCharacterBase* InPlayer);

	// Phase 0：选中三选一中的某项（0-2）
	UFUNCTION(BlueprintCallable, Category = "Sacrifice")
	void SelectSacrificeOption(int32 OptionIndex);

	// Phase 1：确认代价，授予符文并激活 FA 代价逻辑
	UFUNCTION(BlueprintCallable, Category = "Sacrifice")
	void ConfirmSacrifice();

	// 取消（任意阶段）：Phase 1 取消时回到 Phase 0 重新选，Phase 0 取消时关闭
	UFUNCTION(BlueprintCallable, Category = "Sacrifice")
	void CancelSacrifice();

	// BP 事件：展示三个献祭选项（Phase 0）
	UFUNCTION(BlueprintImplementableEvent, Category = "Sacrifice")
	void OnShowSacrificeOptions(const TArray<FAltarSacrificeEntry>& Options);

	// BP 事件：展示代价确认面板（Phase 1）
	UFUNCTION(BlueprintImplementableEvent, Category = "Sacrifice")
	void OnShowCostConfirmation(const FAltarSacrificeEntry& SelectedEntry);

	// BP 事件：献祭结束（bConfirmed = 用户确认 or 取消）
	UFUNCTION(BlueprintImplementableEvent, Category = "Sacrifice")
	void OnSacrificeFinished(bool bConfirmed);

protected:
	virtual void NativeOnActivated() override;
	virtual void NativeOnDeactivated() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override;

	TWeakObjectPtr<APlayerCharacterBase> OwningPlayer;
	TObjectPtr<UAltarDataAsset>          AltarData;
	TArray<FAltarSacrificeEntry>         CurrentOptions;
	int32                                SelectedOptionIndex = -1;
	int32                                Phase              = 0; // 0 = 选选项，1 = 确认代价
};
