#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Types/FlowDataPinProperties.h"
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

	// 要监听的事件 Tag — 收到此 Tag 的 GameplayEvent 时触发 Out 引脚
	UPROPERTY(EditAnywhere, Category = "BuffFlow", meta = (DisplayName = "事件 Tag"))
	FGameplayTag EventTag;

	// 监听目标 — 在哪个 Actor 的 ASC 上监听事件，默认 BuffOwner（玩家自身）
	UPROPERTY(EditAnywhere, Category = "BuffFlow", meta = (DisplayName = "监听目标"))
	EBFTargetSelector Target = EBFTargetSelector::BuffOwner;

	// 事件强度（数据输出引脚）— 每次事件触发时写入 Magnitude 值，可连线到下游节点
	UPROPERTY(EditAnywhere, Category = "Output|Data", meta = (DisplayName = "事件强度（输出）"))
	FFlowDataPinOutputProperty_Float EventMagnitude;

	FGameplayTag GetRuntimeEventTag() const;

protected:
	virtual void ExecuteBuffFlowInput(const FName& PinName) override;
	virtual void Cleanup() override;

private:
	void HandleGameplayEvent(FGameplayTag Tag, const FGameplayEventData* Payload);

	TWeakObjectPtr<UAbilitySystemComponent> BoundASC;
	FDelegateHandle                         EventDelegateHandle;
};
