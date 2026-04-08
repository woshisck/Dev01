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

	UPROPERTY(EditAnywhere, Category = "BuffFlow")
	FGameplayAttribute Attribute;

	UPROPERTY(EditAnywhere, Category = "BuffFlow")
	EBFTargetSelector Target = EBFTargetSelector::BuffOwner;

	/** 获取到的属性值（数据输出引脚，可连接到 Math Float / Apply Gameplay Effect 等节点） */
	UPROPERTY(EditAnywhere, Category = "BuffFlow")
	FFlowDataPinOutputProperty_Float CachedValue;

protected:
	virtual void ExecuteInput(const FName& PinName) override;
};
