#pragma once

#include "CoreMinimal.h"
#include "BuffFlow/Nodes/BFNode_Base.h"
#include "BuffFlow/BuffFlowTypes.h"
#include "BFNode_RemoveRune.generated.h"

class UYogBuffDefinition;

/**
 * 移除符文效果节点
 * 1. 移除目标 ASC 上所有带有 BuffDefinition.BuffTag 的 GE
 * 2. 如果 BuffDefinition 配置了 BuffFlowAsset，在目标 BuffFlowComponent 上停止对应 Flow
 *    （注：停止所有同 FlowAsset 的实例，因为 BFNode_AddRune 使用的 Guid 未持久化）
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "移除符文效果", Category = "BuffFlow|增益"))
class DEVKIT_API UBFNode_RemoveRune : public UBFNode_Base
{
	GENERATED_UCLASS_BODY()

	/** 要移除的符文效果定义 */
	UPROPERTY(EditAnywhere, Category = "BuffFlow")
	TObjectPtr<UYogBuffDefinition> BuffDefinition;

	/** 目标选择 */
	UPROPERTY(EditAnywhere, Category = "BuffFlow")
	EBFTargetSelector Target = EBFTargetSelector::BuffOwner;

protected:
	virtual void ExecuteInput(const FName& PinName) override;
};
