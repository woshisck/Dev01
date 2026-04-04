#pragma once

#include "CoreMinimal.h"
#include "BuffFlow/Nodes/BFNode_Base.h"
#include "BuffFlow/BuffFlowTypes.h"
#include "BFNode_AddRune.generated.h"

class UYogBuffDefinition;

/**
 * 添加符文效果节点
 * 1. 将 BuffDefinition 转为 GE 施加到目标 ASC
 * 2. 如果 BuffDefinition 配置了 BuffFlowAsset，同时在目标 BuffFlowComponent 上启动 Flow
 *
 * Out    — 成功
 * Failed — 目标无效或无 ASC
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "Add Rune Effect", Category = "BuffFlow|Effect"))
class DEVKIT_API UBFNode_AddRune : public UBFNode_Base
{
	GENERATED_UCLASS_BODY()

	/** 符文效果定义 */
	UPROPERTY(EditAnywhere, Category = "BuffFlow")
	TObjectPtr<UYogBuffDefinition> BuffDefinition;

	/** GE 等级 */
	UPROPERTY(EditAnywhere, Category = "BuffFlow")
	int32 Level = 1;

	/** 效果施加目标 */
	UPROPERTY(EditAnywhere, Category = "BuffFlow")
	EBFTargetSelector Target = EBFTargetSelector::BuffOwner;

protected:
	virtual void ExecuteInput(const FName& PinName) override;
};
