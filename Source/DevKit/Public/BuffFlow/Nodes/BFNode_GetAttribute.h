#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "Types/FlowDataPinProperties.h"
#include "BuffFlow/Nodes/BFNode_Base.h"
#include "BuffFlow/BuffFlowTypes.h"
#include "BFNode_GetAttribute.generated.h"

UCLASS(NotBlueprintable, meta = (DisplayName = "Get Attribute", Category = "BuffFlow|Condition"))
class DEVKIT_API UBFNode_GetAttribute : public UBFNode_Base
{
	GENERATED_UCLASS_BODY()

	// 要读取的属性 — 从下拉列表选择 AttributeSet 中的属性
	UPROPERTY(EditAnywhere, Category = "BuffFlow", meta = (DisplayName = "属性"))
	FGameplayAttribute Attribute;

	// 读取目标 — 从哪个 Actor 的 ASC 上读取属性值，默认 BuffOwner
	UPROPERTY(EditAnywhere, Category = "BuffFlow", meta = (DisplayName = "读取目标"))
	EBFTargetSelector Target = EBFTargetSelector::BuffOwner;

	// 属性值（数据输出引脚）— 可连线到 MathFloat / CompareFloat / ApplyAttributeModifier.Value 等
	UPROPERTY(EditAnywhere, Category = "BuffFlow", meta = (DisplayName = "属性值（输出）"))
	FFlowDataPinOutputProperty_Float CachedValue;

protected:
	virtual void ExecuteInput(const FName& PinName) override;
};
