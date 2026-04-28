#pragma once

#include "CoreMinimal.h"
#include "BuffFlow/Nodes/BFNode_Base.h"
#include "BFNode_DisableCells.generated.h"

/**
 * BFNode_DisableCells — 禁用背包格子节点
 *
 * In   → 禁用 CellsToDisable 中的所有格子，随后触发 Out
 * Stop → 手动解除禁用（FA 停止时 Cleanup 也会自动解除）
 *
 * 禁用后：
 *   - 玩家无法将符文拖放到这些格子
 *   - 已放置在这些格子的符文不受影响（仍保持原状）
 *
 * CellsToDisable 中：X = 列（Column），Y = 行（Row），从 0 开始，
 * 范围不超过背包 GridWidth-1 / GridHeight-1。
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "Disable Backpack Cells", Category = "BuffFlow|Backpack"))
class DEVKIT_API UBFNode_DisableCells : public UBFNode_Base
{
	GENERATED_UCLASS_BODY()

	/**
	 * 要禁用的背包格子（X = 列 Column，Y = 行 Row）
	 * In 引脚触发时禁用，Stop 引脚 / FA 停止时自动恢复
	 */
	UPROPERTY(EditAnywhere, Category = "Cells",
		meta = (DisplayName = "Cells To Disable (X=Col Y=Row)"))
	TArray<FIntPoint> CellsToDisable;

protected:
	virtual void ExecuteInput(const FName& PinName) override;
	virtual void Cleanup() override;

private:
	void ApplyDisable();
	void ApplyEnable();
};
