#include "BuffFlow/Nodes/BFNode_SendGameplayEvent.h"
#include "AbilitySystemComponent.h"

UBFNode_SendGameplayEvent::UBFNode_SendGameplayEvent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("BuffFlow|Tag");
#endif
	OutputPins = { FFlowPin(TEXT("Out")), FFlowPin(TEXT("Failed")) };
}

void UBFNode_SendGameplayEvent::ExecuteInput(const FName& PinName)
{
	if (!EventTag.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("[BFNode_SendGameplayEvent] Failed: EventTag 无效"));
		TriggerOutput(TEXT("Failed"), true);
		return;
	}

	AActor* TargetActor = ResolveTarget(Target);
	if (!TargetActor)
	{
		UE_LOG(LogTemp, Warning, TEXT("[BFNode_SendGameplayEvent] Failed: Target(%s) 解析为 null，LastDamageTarget 可能未设置"),
			*EventTag.ToString());
		TriggerOutput(TEXT("Failed"), true);
		return;
	}

	UAbilitySystemComponent* TargetASC = TargetActor->FindComponentByClass<UAbilitySystemComponent>();
	if (!TargetASC)
	{
		UE_LOG(LogTemp, Warning, TEXT("[BFNode_SendGameplayEvent] Failed: Target(%s) 没有 ASC 组件"),
			*TargetActor->GetName());
		TriggerOutput(TEXT("Failed"), true);
		return;
	}

	AActor* InstigatorActor = ResolveTarget(Instigator);
	UE_LOG(LogTemp, Warning, TEXT("[BFNode_SendGameplayEvent] 发送事件 Tag=%s → Target=%s | Instigator=%s"),
		*EventTag.ToString(),
		*TargetActor->GetName(),
		InstigatorActor ? *InstigatorActor->GetName() : TEXT("NULL"));

	FGameplayEventData EventData;
	EventData.EventTag   = EventTag;
	EventData.Instigator = InstigatorActor;
	EventData.Target     = TargetActor;

	// 直接调用已找到的 ASC，避免 SendGameplayEventToActor 内部再次通过
	// IAbilitySystemInterface 查找 ASC（若目标未实现该接口会悄悄返回 0）
	TargetASC->HandleGameplayEvent(EventTag, &EventData);

	TriggerOutput(TEXT("Out"), true);
}
