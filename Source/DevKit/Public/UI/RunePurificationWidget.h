#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "Component/BackpackGridComponent.h"
#include "RunePurificationWidget.generated.h"

class APlayerCharacterBase;

UCLASS(Abstract, Blueprintable)
class DEVKIT_API URunePurificationWidget : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	// 打开前调用，注入玩家引用并进入 Phase 0（符文选择）
	void Setup(APlayerCharacterBase* InPlayer);

	// Phase 0：玩家从符文列表点选一个符文
	UFUNCTION(BlueprintCallable, Category = "Purification")
	void SelectRune(FGuid RuneGuid);

	// Phase 1：玩家选中要删除的格子（本地坐标，必须不为 (0,0)）
	UFUNCTION(BlueprintCallable, Category = "Purification")
	void SelectCell(FIntPoint LocalCell);

	// 确认删除（Phase 1 时有效）
	UFUNCTION(BlueprintCallable, Category = "Purification")
	void ConfirmPurification();

	// 取消（任意阶段均可调用）
	UFUNCTION(BlueprintCallable, Category = "Purification")
	void CancelPurification();

	// BP 事件：展示背包中所有符文（Phase 0 入口）
	UFUNCTION(BlueprintImplementableEvent, Category = "Purification")
	void OnShowRuneList(const TArray<FPlacedRune>& PlacedRunes);

	// BP 事件：展示格子选择面板
	// SelectableCells = Shape.Cells 中除 (0,0) 之外的所有本地坐标
	UFUNCTION(BlueprintImplementableEvent, Category = "Purification")
	void OnShowCellSelection(FGuid RuneGuid, const TArray<FIntPoint>& SelectableCells);

	// BP 事件：净化结束（成功 or 取消）
	UFUNCTION(BlueprintImplementableEvent, Category = "Purification")
	void OnPurificationFinished(bool bSuccess);

	UFUNCTION(BlueprintImplementableEvent, Category = "Purification")
	void OnNativeFocusIndexChanged(int32 FocusPhase, int32 FocusIndex);

protected:
	virtual void NativeOnActivated() override;
	virtual void NativeOnDeactivated() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnAnalogValueChanged(const FGeometry& InGeometry, const FAnalogInputEvent& InAnalogInputEvent) override;
	virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override;

	TWeakObjectPtr<APlayerCharacterBase> OwningPlayer;
	FGuid     SelectedRuneGuid;
	FIntPoint SelectedCell = FIntPoint(0, 0);
	int32     Phase        = 0; // 0 = 选符文，1 = 选格子
	TArray<FPlacedRune> CachedPlacedRunes;
	TArray<FIntPoint> CachedSelectableCells;
	int32 FocusedIndex = 0;
	float LastAnalogNavigationTime = 0.f;

private:
	void MoveFocus(int32 Direction);
	void ActivateFocusedEntry();
	int32 GetCurrentFocusCount() const;
};
