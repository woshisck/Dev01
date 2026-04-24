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

	UPROPERTY(EditAnywhere, Category = "Base")
	EBFDamageBaseMode BaseMode = EBFDamageBaseMode::Flat;

	/** Flat=直接伤害值，Percent=百分比 0~1（如 0.07=7%） */
	UPROPERTY(EditAnywhere, Category = "Base")
	FFlowDataPinInputProperty_Float BaseValue;

	UPROPERTY(EditAnywhere, Category = "Base")
	EBFTargetSelector Target = EBFTargetSelector::LastDamageTarget;

	// ─── 条件乘算（最多3组）─────────────────────────────────

	UPROPERTY(EditAnywhere, Category = "Multiplier", meta = (DisplayName = "Slot 1 Tag"))
	FGameplayTag MultTag1;

	UPROPERTY(EditAnywhere, Category = "Multiplier", meta = (DisplayName = "Slot 1 Condition",
		EditCondition = "MultTag1.IsValid()", EditConditionHides))
	EBFTagConditionMode MultCondition1 = EBFTagConditionMode::HasTag;

	UPROPERTY(EditAnywhere, Category = "Multiplier", meta = (DisplayName = "Slot 1 Multiplier",
		EditCondition = "MultTag1.IsValid()", EditConditionHides))
	float MultValue1 = 1.15f;

	UPROPERTY(EditAnywhere, Category = "Multiplier", meta = (DisplayName = "Slot 2 Tag"))
	FGameplayTag MultTag2;

	UPROPERTY(EditAnywhere, Category = "Multiplier", meta = (DisplayName = "Slot 2 Condition",
		EditCondition = "MultTag2.IsValid()", EditConditionHides))
	EBFTagConditionMode MultCondition2 = EBFTagConditionMode::HasTag;

	UPROPERTY(EditAnywhere, Category = "Multiplier", meta = (DisplayName = "Slot 2 Multiplier",
		EditCondition = "MultTag2.IsValid()", EditConditionHides))
	float MultValue2 = 1.15f;

	UPROPERTY(EditAnywhere, Category = "Multiplier", meta = (DisplayName = "Slot 3 Tag"))
	FGameplayTag MultTag3;

	UPROPERTY(EditAnywhere, Category = "Multiplier", meta = (DisplayName = "Slot 3 Condition",
		EditCondition = "MultTag3.IsValid()", EditConditionHides))
	EBFTagConditionMode MultCondition3 = EBFTagConditionMode::HasTag;

	UPROPERTY(EditAnywhere, Category = "Multiplier", meta = (DisplayName = "Slot 3 Multiplier",
		EditCondition = "MultTag3.IsValid()", EditConditionHides))
	float MultValue3 = 1.15f;

	// ─── 输出数据引脚 ───────────────────────────────────────

	UPROPERTY(EditAnywhere, Category = "Output|Data")
	FFlowDataPinOutputProperty_Float FinalDamage;

protected:
	virtual void ExecuteInput(const FName& PinName) override;
};