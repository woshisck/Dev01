#pragma once

#include "CoreMinimal.h"
#include "Types/FlowDataPinProperties.h"
#include "BuffFlow/Nodes/BFNode_Base.h"
#include "BFNode_GetRuneTuningValue.generated.h"

UCLASS(NotBlueprintable, meta = (DisplayName = "Get Rune Tuning Value", Category = "BuffFlow|Rune"))
class DEVKIT_API UBFNode_GetRuneTuningValue : public UBFNode_Base
{
	GENERATED_UCLASS_BODY()

	// 数值表 Key — 在符文 DA 的调参表中查找此名称对应的值
	UPROPERTY(EditAnywhere, Category = "BuffFlow|Rune", meta = (DisplayName = "数值表 Key"))
	FName Key;

	// 默认值 — Key 不存在时返回的兜底值
	UPROPERTY(EditAnywhere, Category = "BuffFlow|Rune", meta = (DisplayName = "默认值"))
	float DefaultValue = 0.f;

	// 查询结果（数据输出引脚）— 可连线到 ApplyAttributeModifier.Value 等
	UPROPERTY(EditAnywhere, Category = "BuffFlow|Rune", meta = (DisplayName = "数值（输出）"))
	FFlowDataPinOutputProperty_Float Value;

	// 是否找到（数据输出引脚）— Key 在调参表中存在时为 true
	UPROPERTY(EditAnywhere, Category = "BuffFlow|Rune", meta = (DisplayName = "是否找到（输出）"))
	FFlowDataPinOutputProperty_Bool bFound;

protected:
	virtual void ExecuteInput(const FName& PinName) override;
};
