#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Types/FlowDataPinProperties.h"
#include "BuffFlow/Nodes/BFNode_Base.h"
#include "BuffFlow/BuffFlowTypes.h"
#include "BFNode_SendGameplayEvent.generated.h"

/**
 * 向目标发送 GameplayEvent，触发目标 ASC 上监听该 Tag 的 GA。
 *
 * 典型用法：FA_Rune_Knockback 在 OnDamageDealt 后发送 Ability.Knockback
 * 给被击目标，触发目标身上的 BGA_Knockback（已配置 AbilityTriggers 监听该 Event）。
 *
 * Target     — 接收事件的 Actor（被触发 GA 的对象，通常为 LastDamageTarget）
 * Instigator — GA 内部用于计算方向等的发起者（通常为 DamageCauser / BuffOwner）
 *
 * Out    — 事件发送成功
 * Failed — Target 无效或无 ASC
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "Send Gameplay Event", Category = "BuffFlow|Tag"))
class DEVKIT_API UBFNode_SendGameplayEvent : public UBFNode_Base
{
	GENERATED_UCLASS_BODY()

	// 要发送的事件 Tag — 目标 GA 的 AbilityTriggers 需监听此 Tag
	UPROPERTY(EditAnywhere, Category = "BuffFlow", meta = (DisplayName = "事件 Tag"))
	FGameplayTag EventTag;

	// 事件接收目标 — GA 在此 Actor 的 ASC 上被触发（通常为 LastDamageTarget）
	UPROPERTY(EditAnywhere, Category = "BuffFlow", meta = (DisplayName = "事件接收目标"))
	EBFTargetSelector Target = EBFTargetSelector::LastDamageTarget;

	// Payload 目标 — FGameplayEventData.Target 字段（无特殊需求时与 Target 保持一致）
	UPROPERTY(EditAnywhere, Category = "BuffFlow", meta = (DisplayName = "Payload 目标"))
	EBFTargetSelector PayloadTarget = EBFTargetSelector::LastDamageTarget;

	// 事件发起者 — GA 内部通过 TriggerEventData.Instigator 读取（通常为 DamageCauser）
	UPROPERTY(EditAnywhere, Category = "BuffFlow", meta = (DisplayName = "发起者"))
	EBFTargetSelector Instigator = EBFTargetSelector::DamageCauser;

	// 事件强度值 — 写入 FGameplayEventData.EventMagnitude；可连接 Pure 数据节点输出
	UPROPERTY(EditAnywhere, Category = "BuffFlow", meta = (DisplayName = "事件强度值"))
	FFlowDataPinInputProperty_Float Magnitude;

protected:
	virtual void ExecuteInput(const FName& PinName) override;
};
