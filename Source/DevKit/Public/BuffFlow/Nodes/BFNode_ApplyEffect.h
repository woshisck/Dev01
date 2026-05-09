#pragma once

#include "CoreMinimal.h"
#include "Types/FlowDataPinProperties.h"
#include "BuffFlow/Nodes/BFNode_Base.h"
#include "BuffFlow/BuffFlowTypes.h"
#include "GameplayEffect.h"
#include "GameplayTagContainer.h"
#include "BFNode_ApplyEffect.generated.h"

/**
 * 实现效果节点：向指定目标施加一个 GameplayEffect Class。
 * Target 默认为"上次伤害目标"，适合配合伤害触发器使用。
 *
 * Level 为数据引脚，可从上游节点（如 GetRuneInfo.Level）连线驱动，
 * 也可直接在节点上填写固定值。
 *
 * SetByCaller 槽位（最多 3 个）：
 *   Tag 留空则该槽位跳过；Value 可接受数据引脚驱动。
 *   GE 中需提前用对应 Tag 声明 SetByCaller Magnitude。
 *
 * 输出数据引脚（施加时写入，可由后续节点读取）：
 *   bGEApplied    — 是否成功施加
 *   GEStackCount  — 施加后当前层数（HasDuration/Infinite Stackable GE）
 *   GELevel       — GE 等级
 *   GETimeRemaining — 剩余时间（秒），Infinite GE 返回 -1
 *
 * 注意：输出引脚反映施加瞬间的状态，不随时间动态更新。
 *
 * Remove 引脚：
 *   触发后按 RemoveMode 移除 GE 堆叠层数：
 *   AllStacks   — 移除所有层（整个 GE 消失）
 *   OneStack    — 移除 1 层
 *   CustomCount — 移除 StacksToRemove 数据引脚指定的层数
 *   移除完成后触发 Removed 输出引脚。
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "Apply Gameplay Effect Class", Category = "BuffFlow|Effect"))
class DEVKIT_API UBFNode_ApplyEffect : public UBFNode_Base
{
	GENERATED_UCLASS_BODY()

	// 要施加的 GameplayEffect 类
	UPROPERTY(EditAnywhere, Category = "Effect", meta = (DisplayName = "要施加的效果类"))
	TSubclassOf<UGameplayEffect> Effect;

	// 效果等级（数据引脚）— 可直接填写固定值，也可连接上游数据引脚动态驱动
	UPROPERTY(EditAnywhere, Category = "Effect", meta = (DisplayName = "效果等级"))
	FFlowDataPinInputProperty_Float Level;

	// 施加目标（默认：上次伤害目标）
	UPROPERTY(EditAnywhere, Category = "Effect", meta = (DisplayName = "施加目标"))
	EBFTargetSelector Target = EBFTargetSelector::LastDamageTarget;

	// 施加次数 — 对目标连续施加 GE 的次数，适用于叠层毒/燃等状态效果
	UPROPERTY(EditAnywhere, Category = "Effect", meta = (ClampMin = "1", ClampMax = "20", DisplayName = "施加次数"))
	int32 ApplicationCount = 1;

	// FA结束时移除效果 — 关闭后 GE 在 FA 停止后仍保持自身持续时间，适用于 DOT/状态效果
	UPROPERTY(EditAnywhere, Category = "Effect|Lifecycle", meta = (DisplayName = "FA结束时移除效果"))
	bool bRemoveEffectOnCleanup = true;

	// ─── SetByCaller 槽位 ─────────────────────────────────────────
	// GE 中用对应 Tag 声明 SetByCaller 数值槽，FA 节点在此处填写实际值。
	// Tag 留空 → 该槽位自动跳过。

	// 槽位1 Tag — GE 中声明的 SetByCaller Tag
	UPROPERTY(EditAnywhere, Category = "SetByCaller", meta = (DisplayName = "槽位1 Tag"))
	FGameplayTag SetByCallerTag1;

	// 槽位1 数值 — 传入 SetByCallerTag1 对应槽的实际值（可连接数据引脚）
	UPROPERTY(EditAnywhere, Category = "SetByCaller", meta = (DisplayName = "槽位1 数值",
		EditCondition = "SetByCallerTag1.IsValid()", EditConditionHides))
	FFlowDataPinInputProperty_Float SetByCallerValue1;

	// 槽位2 Tag
	UPROPERTY(EditAnywhere, Category = "SetByCaller", meta = (DisplayName = "槽位2 Tag"))
	FGameplayTag SetByCallerTag2;

	// 槽位2 数值
	UPROPERTY(EditAnywhere, Category = "SetByCaller", meta = (DisplayName = "槽位2 数值",
		EditCondition = "SetByCallerTag2.IsValid()", EditConditionHides))
	FFlowDataPinInputProperty_Float SetByCallerValue2;

	// 槽位3 Tag
	UPROPERTY(EditAnywhere, Category = "SetByCaller", meta = (DisplayName = "槽位3 Tag"))
	FGameplayTag SetByCallerTag3;

	// 槽位3 数值
	UPROPERTY(EditAnywhere, Category = "SetByCaller", meta = (DisplayName = "槽位3 数值",
		EditCondition = "SetByCallerTag3.IsValid()", EditConditionHides))
	FFlowDataPinInputProperty_Float SetByCallerValue3;

	// ─── Remove 配置 ─────────────────────────────────────────────────

	// 移除模式 — Remove引脚触发时按此模式清除GE层数
	UPROPERTY(EditAnywhere, Category = "Remove", meta = (DisplayName = "移除模式"))
	EBFRemoveMode RemoveMode = EBFRemoveMode::AllStacks;

	// 移除层数 — CustomCount模式下指定移除的层数（可连接数据引脚）
	UPROPERTY(EditAnywhere, Category = "Remove", meta = (DisplayName = "移除层数",
		EditCondition = "RemoveMode == EBFRemoveMode::CustomCount", EditConditionHides))
	FFlowDataPinInputProperty_Int32 StacksToRemove;

	// ─── 输出数据引脚（施加时写入） ───────────────────────────────────

	// 是否成功施加（输出）— Instant GE 也会返回 true
	UPROPERTY(EditAnywhere, Category = "Output|GEInfo", meta = (DisplayName = "是否成功施加（输出）"))
	FFlowDataPinOutputProperty_Bool bGEApplied;

	// 当前层数（输出）— 施加后的 GE 层数，非堆叠 GE 固定为 1
	UPROPERTY(EditAnywhere, Category = "Output|GEInfo", meta = (DisplayName = "当前层数（输出）"))
	FFlowDataPinOutputProperty_Int32 GEStackCount;

	// GE等级（输出）
	UPROPERTY(EditAnywhere, Category = "Output|GEInfo", meta = (DisplayName = "GE等级（输出）"))
	FFlowDataPinOutputProperty_Float GELevel;

	// 剩余时间（输出）— Infinite GE 返回 -1；Instant GE 返回 0；HasDuration 返回完整时长
	UPROPERTY(EditAnywhere, Category = "Output|GEInfo", meta = (DisplayName = "剩余时间（输出）"))
	FFlowDataPinOutputProperty_Float GETimeRemaining;

protected:
	virtual void ExecuteInput(const FName& PinName) override;

	/** FA 停止时自动调用，移除由此节点施加的 GE（Instant GE handle 无效，自动跳过） */
	virtual void Cleanup() override;

private:
	/** 首次施加后存储的 handle（Unique/Stackable GE 共享同一实例，handle 保持有效） */
	FActiveGameplayEffectHandle GrantedHandle;
	TWeakObjectPtr<UAbilitySystemComponent> GrantedASC;

	/** Cleanup 主动移除 GE 时置 true，阻止 HandleGERemoved 二次触发 OnRemoved 引脚 */
	bool bCleaningUp = false;

	/** GE 被外部（时间到期/手动移除）撤销时的回调，触发 OnRemoved 执行引脚 */
	void HandleGERemoved(const FGameplayEffectRemovalInfo& RemovalInfo);
};
