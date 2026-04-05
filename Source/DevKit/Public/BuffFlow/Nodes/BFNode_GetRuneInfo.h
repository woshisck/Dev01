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
 *
 * 数据输出引脚（可连入 CompareFloat.A 等）：
 *   bIsActive     — 是否活跃
 *   StackCount    — 当前叠加层数（Stack 模式 >= 1，其余固定为 1）
 *   Level         — GE 等级
 *   TimeRemaining — 剩余持续时间（秒），-1 = 永久
 *
 * 典型用法（狂暴：超过 5 层触发额外效果）：
 *   GetRuneInfo(Target=Self, RuneTag=Buff.Berserk)
 *     ├─ Found → CompareFloat(StackCount >= 5) → True → AddEffect(+10% ATK)
 *     └─ NotFound → [skip]
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "Get Rune Info", Category = "BuffFlow|Condition"))
class DEVKIT_API UBFNode_GetRuneInfo : public UBFNode_Base
{
	GENERATED_UCLASS_BODY()

	/** 要查询的目标 */
	UPROPERTY(EditAnywhere, Category = "BuffFlow")
	EBFTargetSelector Target = EBFTargetSelector::BuffOwner;

	/** DA 中 RuneConfig.RuneTag 填写的 Tag */
	UPROPERTY(EditAnywhere, Category = "BuffFlow")
	FGameplayTag RuneTag;

	// ---- 数据输出引脚 ----

	/** 符文 GE 是否活跃 */
	UPROPERTY(EditAnywhere, Category = "BuffFlow")
	FFlowDataPinOutputProperty_Bool bIsActive;

	/** 当前叠加层数 */
	UPROPERTY(EditAnywhere, Category = "BuffFlow")
	FFlowDataPinOutputProperty_Int32 StackCount;

	/** GE 等级 */
	UPROPERTY(EditAnywhere, Category = "BuffFlow")
	FFlowDataPinOutputProperty_Float Level;

	/** 剩余时间（秒），-1 = 永久 */
	UPROPERTY(EditAnywhere, Category = "BuffFlow")
	FFlowDataPinOutputProperty_Float TimeRemaining;

protected:
	virtual void ExecuteInput(const FName& PinName) override;
};
