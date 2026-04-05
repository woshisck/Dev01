#pragma once

#include "CoreMinimal.h"
#include "BuffFlow/Nodes/BFNode_Base.h"
#include "BuffFlow/BuffFlowTypes.h"
#include "BFNode_RemoveRune.generated.h"

class URuneDataAsset;

/**
 * 移除符文效果节点
 * 1. 移除目标 ASC 上所有带有 RuneAsset.RuneTemplate.BuffTag 的 GE
 * 2. 如果 RuneAsset 配置了 BuffFlowAsset，在目标 BuffFlowComponent 上停止对应 Flow
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "Remove Rune Effect", Category = "BuffFlow|Effect"))
class DEVKIT_API UBFNode_RemoveRune : public UBFNode_Base
{
	GENERATED_UCLASS_BODY()

	/** 要移除的符文效果定义（DA_Rune_xxx） */
	UPROPERTY(EditAnywhere, Category = "BuffFlow")
	TObjectPtr<URuneDataAsset> RuneAsset;

	/** 目标选择 */
	UPROPERTY(EditAnywhere, Category = "BuffFlow")
	EBFTargetSelector Target = EBFTargetSelector::BuffOwner;

protected:
	virtual void ExecuteInput(const FName& PinName) override;
};
