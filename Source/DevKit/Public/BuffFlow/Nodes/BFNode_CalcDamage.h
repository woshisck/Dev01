#pragma once

#include "CoreMinimal.h"
#include "Types/FlowDataPinProperties.h"
#include "BuffFlow/Nodes/BFNode_Base.h"
#include "BuffFlow/BuffFlowTypes.h"
#include "GameplayTagContainer.h"
#include "BFNode_CalcDamage.generated.h"

UENUM(BlueprintType)
enum class EBFDamageBaseMode : uint8
{
	Flat              UMETA(DisplayName = "固定值"),
	PercentMaxHP      UMETA(DisplayName = "%最大生命"),
	PercentCurrentHP  UMETA(DisplayName = "%当前生命"),
	PercentMissingHP  UMETA(DisplayName = "%已损失生命"),
};

UENUM(BlueprintType)
enum class EBFTagConditionMode : uint8
{
	HasTag     UMETA(DisplayName = "有此Tag时"),
	MissingTag UMETA(DisplayName = "无此Tag时"),
};

UCLASS(NotBlueprintable, meta = (DisplayName = "Calc Damage", Category = "BuffFlow|Damage"))
class DEVKIT_API UBFNode_CalcDamage : public UBFNode_Base
{
	GENERATED_UCLASS_BODY()

	// ─── 基础值 ─────────────────────────────────────────────

	// 基础值模式 — 固定值/百分比最大生命/当前生命/已损失生命
	UPROPERTY(EditAnywhere, Category = "Base", meta = (DisplayName = "基础值模式"))
	EBFDamageBaseMode BaseMode = EBFDamageBaseMode::Flat;

	// 基础数值 — Flat=直接伤害值；Percent=百分比 0~1（如 0.07=7%）
	UPROPERTY(EditAnywhere, Category = "Base", meta = (DisplayName = "基础数值"))
	FFlowDataPinInputProperty_Float BaseValue;

	// 目标 — 百分比模式下读取生命值的目标角色
	UPROPERTY(EditAnywhere, Category = "Base", meta = (DisplayName = "目标"))
	EBFTargetSelector Target = EBFTargetSelector::LastDamageTarget;

	// ─── 条件乘算（最多3组）─────────────────────────────────

	// 乘算槽1 Tag — 检查目标是否拥有此Tag，满足条件则额外乘以槽1倍率
	UPROPERTY(EditAnywhere, Category = "Multiplier", meta = (DisplayName = "乘算槽1 Tag"))
	FGameplayTag MultTag1;

	// 槽1 条件 — 有此Tag时/无此Tag时触发乘算
	UPROPERTY(EditAnywhere, Category = "Multiplier", meta = (DisplayName = "槽1 条件",
		EditCondition = "MultTag1.IsValid()", EditConditionHides))
	EBFTagConditionMode MultCondition1 = EBFTagConditionMode::HasTag;

	// 槽1 倍率 — 条件满足时乘以此值（如 1.15 = +15% 伤害）
	UPROPERTY(EditAnywhere, Category = "Multiplier", meta = (DisplayName = "槽1 倍率",
		EditCondition = "MultTag1.IsValid()", EditConditionHides))
	float MultValue1 = 1.15f;

	// 乘算槽2 Tag
	UPROPERTY(EditAnywhere, Category = "Multiplier", meta = (DisplayName = "乘算槽2 Tag"))
	FGameplayTag MultTag2;

	// 槽2 条件
	UPROPERTY(EditAnywhere, Category = "Multiplier", meta = (DisplayName = "槽2 条件",
		EditCondition = "MultTag2.IsValid()", EditConditionHides))
	EBFTagConditionMode MultCondition2 = EBFTagConditionMode::HasTag;

	// 槽2 倍率
	UPROPERTY(EditAnywhere, Category = "Multiplier", meta = (DisplayName = "槽2 倍率",
		EditCondition = "MultTag2.IsValid()", EditConditionHides))
	float MultValue2 = 1.15f;

	// 乘算槽3 Tag
	UPROPERTY(EditAnywhere, Category = "Multiplier", meta = (DisplayName = "乘算槽3 Tag"))
	FGameplayTag MultTag3;

	// 槽3 条件
	UPROPERTY(EditAnywhere, Category = "Multiplier", meta = (DisplayName = "槽3 条件",
		EditCondition = "MultTag3.IsValid()", EditConditionHides))
	EBFTagConditionMode MultCondition3 = EBFTagConditionMode::HasTag;

	// 槽3 倍率
	UPROPERTY(EditAnywhere, Category = "Multiplier", meta = (DisplayName = "槽3 倍率",
		EditCondition = "MultTag3.IsValid()", EditConditionHides))
	float MultValue3 = 1.15f;

	// ─── 输出数据引脚 ───────────────────────────────────────

	// 最终伤害（输出）— 计算完成后写入此引脚，可连接到 DoDamage.Damage 等节点
	UPROPERTY(EditAnywhere, Category = "Output|Data", meta = (DisplayName = "最终伤害（输出）"))
	FFlowDataPinOutputProperty_Float FinalDamage;

protected:
	virtual void ExecuteInput(const FName& PinName) override;
};
