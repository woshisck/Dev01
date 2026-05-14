#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Types/FlowDataPinProperties.h"
#include "BuffFlow/Nodes/BFNode_Base.h"
#include "BuffFlow/BuffFlowTypes.h"
#include "BFNode_GetRuneInfo.generated.h"

/**
 * 查询目标 ASC 上匹配 RuneTag 的符文（GE）的运行时状态，
 * 输出数据引脚供 CompareFloat 等节点直接连线使用。
 *
 * 执行输出引脚：
 *   Found    — 符文 GE 在目标上活跃（进入 Found 分支后可读取数据引脚）
 *   NotFound — 符文 GE 不在目标上
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "Get Rune Info", Category = "BuffFlow|Condition"))
class DEVKIT_API UBFNode_GetRuneInfo : public UBFNode_Base
{
	GENERATED_UCLASS_BODY()

	// 查询目标 — 在哪个 Actor 的 ASC 上查询符文 GE 状态
	UPROPERTY(EditAnywhere, Category = "BuffFlow", meta = (DisplayName = "查询目标"))
	EBFTargetSelector Target = EBFTargetSelector::BuffOwner;

	// 符文 Tag — DA 中 RuneConfig.RuneTag 填写的 Tag，用于定位对应 GE
	UPROPERTY(EditAnywhere, Category = "BuffFlow", meta = (DisplayName = "符文 Tag"))
	FGameplayTag RuneTag;

	// 是否活跃（数据输出引脚）
	UPROPERTY(EditAnywhere, Category = "BuffFlow", meta = (DisplayName = "是否活跃（输出）"))
	FFlowDataPinOutputProperty_Bool bIsActive;

	// 当前叠加层数（数据输出引脚）— 非堆叠 GE 固定为 1
	UPROPERTY(EditAnywhere, Category = "BuffFlow", meta = (DisplayName = "层数（输出）"))
	FFlowDataPinOutputProperty_Int32 StackCount;

	// GE 等级（数据输出引脚）
	UPROPERTY(EditAnywhere, Category = "BuffFlow", meta = (DisplayName = "等级（输出）"))
	FFlowDataPinOutputProperty_Float Level;

	// 剩余时间（数据输出引脚）— 秒；-1 = 永久；0 = 瞬发 GE
	UPROPERTY(EditAnywhere, Category = "BuffFlow", meta = (DisplayName = "剩余时间（输出）"))
	FFlowDataPinOutputProperty_Float TimeRemaining;

protected:
	virtual void ExecuteBuffFlowInput(const FName& PinName) override;
};
