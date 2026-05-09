#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "GameplayEffectExecutionCalculation.h"
#include "Types/FlowDataPinProperties.h"
#include "BuffFlow/Nodes/BFNode_Base.h"
#include "BuffFlow/BuffFlowTypes.h"
#include "BFNode_ApplyExecution.generated.h"

/**
 * 在 FA 里直接执行 C++ GameplayEffectExecutionCalculation，无需手动创建 Blueprint GE 资产。
 *
 * 工作原理：
 *   节点在运行时动态构建一个临时 Instant GE，把 ExecCalcClass 挂上去，
 *   再通过 SetByCaller 槽传入运行时参数，最后施加到目标。
 *   对使用者来说：你只需要让我（程序）写好 UGameplayEffectExecutionCalculation 子类，
 *   然后在这个节点的 ExecCalcClass 里选择它就行，不需要创建任何 GE .uasset。
 *
 * 典型用途：
 *   - 需要读取多个属性、做复杂计算的伤害公式
 *   - 需要在计算里检查 Tag / Phase 等运行时状态
 *   - 任何 ApplyAttributeModifier 无法表达的"自定义计算逻辑"
 *
 * SetByCaller 槽：
 *   ExecCalc 内部通过 ExecutionParams.AttemptCalculateMagnitudeWithTags 读取，
 *   或者用 Spec.GetSetByCallerMagnitude(Tag) 读取节点传入的值。
 *   Tag 留空 → 该槽自动跳过。
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "Apply Execution Calculation", Category = "BuffFlow|Effect"))
class DEVKIT_API UBFNode_ApplyExecution : public UBFNode_Base
{
	GENERATED_UCLASS_BODY()

	// 执行计算类 — 由程序写好的 C++ ExecutionCalculation 子类，无需创建 Blueprint GE 资产
	UPROPERTY(EditAnywhere, Category = "Execution", meta = (DisplayName = "执行计算类"))
	TSubclassOf<UGameplayEffectExecutionCalculation> ExecCalcClass;

	// 施加目标
	UPROPERTY(EditAnywhere, Category = "Execution", meta = (DisplayName = "施加目标"))
	EBFTargetSelector Target = EBFTargetSelector::LastDamageTarget;

	// ── SetByCaller 槽（最多 3 个，向 ExecCalc 传入运行时浮点参数） ────────
	// ExecCalc 内部通过 Spec.GetSetByCallerMagnitude(Tag) 读取这些值。

	// 槽位1 Tag — ExecCalc 内部读取用的 SetByCaller Tag
	UPROPERTY(EditAnywhere, Category = "SetByCaller", meta = (DisplayName = "槽位1 Tag"))
	FGameplayTag SetByCallerTag1;

	// 槽位1 数值 — 传入 SetByCallerTag1 对应槽的实际值（可连接数据引脚）
	UPROPERTY(EditAnywhere, Category = "SetByCaller",
		meta = (DisplayName = "槽位1 数值",
		        EditCondition = "SetByCallerTag1.IsValid()", EditConditionHides))
	FFlowDataPinInputProperty_Float SetByCallerValue1;

	// 槽位2 Tag
	UPROPERTY(EditAnywhere, Category = "SetByCaller", meta = (DisplayName = "槽位2 Tag"))
	FGameplayTag SetByCallerTag2;

	// 槽位2 数值
	UPROPERTY(EditAnywhere, Category = "SetByCaller",
		meta = (DisplayName = "槽位2 数值",
		        EditCondition = "SetByCallerTag2.IsValid()", EditConditionHides))
	FFlowDataPinInputProperty_Float SetByCallerValue2;

	// 槽位3 Tag
	UPROPERTY(EditAnywhere, Category = "SetByCaller", meta = (DisplayName = "槽位3 Tag"))
	FGameplayTag SetByCallerTag3;

	// 槽位3 数值
	UPROPERTY(EditAnywhere, Category = "SetByCaller",
		meta = (DisplayName = "槽位3 数值",
		        EditCondition = "SetByCallerTag3.IsValid()", EditConditionHides))
	FFlowDataPinInputProperty_Float SetByCallerValue3;

protected:
	virtual void ExecuteInput(const FName& PinName) override;

private:
	/** 缓存动态构建的 GE（ExecCalc 类不变时复用，避免每帧 NewObject） */
	UPROPERTY()
	TObjectPtr<UGameplayEffect> CachedGE;

	/** 上次缓存时使用的 ExecCalcClass，Class 变化时重建 */
	TSubclassOf<UGameplayEffectExecutionCalculation> CachedExecClass;
};
