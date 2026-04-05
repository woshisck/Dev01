#pragma once

#include "CoreMinimal.h"
#include "BuffFlow/Nodes/BFNode_Base.h"
#include "BuffFlow/BuffFlowTypes.h"
#include "GameplayTagContainer.h"
#include "Types/FlowDataPinProperties.h"
#include "BFNode_SendGameplayEvent.generated.h"

/**
 * 向目标发送 GameplayEvent，并将 RuneAsset 作为 OptionalObject 携带。
 *
 * 典型用法：
 *   BFNode_AddRune.CachedRuneAsset → BFNode_SendGameplayEvent.RuneAsset
 *   GA 用 ActivateAbilityFromEvent 接收，从 EventData.OptionalObject 读取 DA Params。
 *
 * Out    — 事件已发送
 * Failed — 目标无效或无 ASC
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "Send Gameplay Event", Category = "BuffFlow|Effect"))
class DEVKIT_API UBFNode_SendGameplayEvent : public UBFNode_Base
{
	GENERATED_UCLASS_BODY()

	/** 事件目标 */
	UPROPERTY(EditAnywhere, Category = "BuffFlow")
	EBFTargetSelector Target = EBFTargetSelector::BuffOwner;

	/** 发送的 GameplayEvent Tag */
	UPROPERTY(EditAnywhere, Category = "BuffFlow")
	FGameplayTag EventTag;

	/**
	 * 携带的 RuneDataAsset（作为 EventData.OptionalObject）。
	 * 可直接在此选择 DA，也可从上游 BFNode_AddRune 的 CachedRuneAsset 引脚连入。
	 */
	UPROPERTY(EditAnywhere, Category = "BuffFlow")
	FFlowDataPinInputProperty_Object RuneAsset;

	/** 附加数值（作为 EventData.EventMagnitude，GA 可直接读取） */
	UPROPERTY(EditAnywhere, Category = "BuffFlow")
	float EventMagnitude = 0.f;

protected:
	virtual void ExecuteInput(const FName& PinName) override;
};
