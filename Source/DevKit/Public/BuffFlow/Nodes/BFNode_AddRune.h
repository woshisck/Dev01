#pragma once

#include "CoreMinimal.h"
#include "BuffFlow/Nodes/BFNode_Base.h"
#include "BuffFlow/BuffFlowTypes.h"
#include "Types/FlowDataPinProperties.h"
#include "BFNode_AddRune.generated.h"

class URuneDataAsset;

/**
 * 添加符文效果节点
 * 1. 将 RuneDataAsset 转为 GE 施加到目标 ASC
 * 2. 如果 RuneDataAsset 配置了 FlowAsset，同时在目标 BuffFlowComponent 上启动 Flow
 *
 * Out    — 成功（CachedRuneAsset 数据引脚可供后续节点读取）
 * Failed — 目标无效或无 ASC
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "Add Rune Effect", Category = "BuffFlow|Effect"))
class DEVKIT_API UBFNode_AddRune : public UBFNode_Base
{
	GENERATED_UCLASS_BODY()

	/** 符文/效果定义（DA_Rune_xxx） */
	UPROPERTY(EditAnywhere, Category = "BuffFlow")
	TObjectPtr<URuneDataAsset> RuneAsset;

	/** GE 等级 */
	UPROPERTY(EditAnywhere, Category = "BuffFlow")
	int32 Level = 1;

	/** 效果施加目标 */
	UPROPERTY(EditAnywhere, Category = "BuffFlow")
	EBFTargetSelector Target = EBFTargetSelector::BuffOwner;

	/** 执行成功后缓存的 RuneAsset，作为数据输出引脚供后续节点（SendGameplayEvent 等）读取 */
	UPROPERTY(EditAnywhere, Category = "BuffFlow")
	FFlowDataPinOutputProperty_Object CachedRuneAsset;

protected:
	virtual void ExecuteInput(const FName& PinName) override;
};
