#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "BuffFlow/Nodes/BFNode_Base.h"
#include "BuffFlow/BuffFlowTypes.h"
#include "BFNode_WaitGameplayEvent.generated.h"

class UAbilitySystemComponent;
struct FGameplayEventData;

/**
 * 等待 GameplayEvent 节点：监听指定 Actor 的 ASC 上的 GameplayEvent，收到时触发 Out 引脚。
 *
 * 典型用途：
 *  · 跨符文通信 — 符文A 通过 Send Gameplay Event（Target=BuffOwner）发送信号，
 *                 符文B 用本节点监听 BuffOwner 上的信号，触发后执行效果。
 *  · 监听任意 ASC 上的事件（Target 可按需配置）。
 *
 * 节点持续监听，每次收到匹配 Tag 的事件都触发 Out 引脚（不会自动停止）。
 * FA 停止时 Cleanup 自动解绑。
 *
 * 事件触发后，Payload 中的 Instigator / Target 会写入 BFC.LastEventContext，
 * 供下游节点通过 DamageCauser / LastDamageTarget 选择器读取。
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "Wait Gameplay Event", Category = "BuffFlow|Tag"))
class DEVKIT_API UBFNode_WaitGameplayEvent : public UBFNode_Base
{
	GENERATED_UCLASS_BODY()

	/** 要监听的 GameplayEvent Tag */
	UPROPERTY(EditAnywhere, Category = "BuffFlow")
	FGameplayTag EventTag;

	/**
	 * 监听哪个 Actor 的 ASC 上的事件。
	 * 默认 BuffOwner（当前角色本身）—— 适合跨符文通信场景：
	 *   符文A 通过 Send Gameplay Event（Target=BuffOwner）发送，
	 *   符文B 用本节点在 BuffOwner 上监听。
	 */
	UPROPERTY(EditAnywhere, Category = "BuffFlow")
	EBFTargetSelector Target = EBFTargetSelector::BuffOwner;

protected:
	virtual void ExecuteInput(const FName& PinName) override;
	virtual void Cleanup() override;

private:
	void HandleGameplayEvent(FGameplayTag Tag, const FGameplayEventData* Payload);

	TWeakObjectPtr<UAbilitySystemComponent> BoundASC;
	FDelegateHandle                         EventDelegateHandle;
};
