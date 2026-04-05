#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Types/FlowDataPinProperties.h"
#include "BuffFlow/Nodes/BFNode_Base.h"
#include "BuffFlow/BuffFlowTypes.h"
#include "BFNode_GetGEInfo.generated.h"

/**
 * 查询目标 ASC 上匹配 BuffTag 的活跃 GE 的运行时信息，输出数据引脚供后续节点使用。
 *
 * 输出引脚：
 *   Found    — GE 在目标上活跃
 *   NotFound — GE 不在目标上
 *
 * 数据输出（可连入 CompareFloat / CompareInt 等节点的输入引脚）：
 *   bIsActive     — 是否活跃（与执行引脚语义一致）
 *   StackCount    — 当前层数
 *   Level         — GE 等级
 *   TimeRemaining — 剩余持续时间（秒），-1 表示永久
 *
 * 典型用法（狂暴示例）：
 *   GetGEInfo(Target=Self, BuffTag=Buff.Berserk)
 *     ├─ Found → CompareFloat(StackCount > 5) → True → AddEffect(+10% ATK)
 *     └─ NotFound → [skip]
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "Get GE Info", Category = "BuffFlow|Condition"))
class DEVKIT_API UBFNode_GetGEInfo : public UBFNode_Base
{
	GENERATED_UCLASS_BODY()

	/** 要查询的目标 */
	UPROPERTY(EditAnywhere, Category = "BuffFlow")
	EBFTargetSelector Target = EBFTargetSelector::BuffOwner;

	/** 要查询的 GE 的 BuffTag（与 RuneConfig.BuffTag 一致） */
	UPROPERTY(EditAnywhere, Category = "BuffFlow")
	FGameplayTag BuffTag;

	// ---- 数据输出引脚 ----

	/** GE 是否在目标上活跃 */
	UPROPERTY(EditAnywhere, Category = "BuffFlow")
	FFlowDataPinOutputProperty_Bool bIsActive;

	/** 当前层数（Stack 模式下 >= 1，非 Stack 模式固定为 1） */
	UPROPERTY(EditAnywhere, Category = "BuffFlow")
	FFlowDataPinOutputProperty_Int32 StackCount;

	/** GE 等级 */
	UPROPERTY(EditAnywhere, Category = "BuffFlow")
	FFlowDataPinOutputProperty_Float Level;

	/**
	 * 剩余持续时间（秒）
	 *  -1 = 永久（Infinite）
	 *   0 = 即将过期或已过期
	 */
	UPROPERTY(EditAnywhere, Category = "BuffFlow")
	FFlowDataPinOutputProperty_Float TimeRemaining;

protected:
	virtual void ExecuteInput(const FName& PinName) override;
};
