#include "BuffFlow/Nodes/BFNode_WaitGameplayEvent.h"
#include "AbilitySystemComponent.h"
#include "BuffFlow/BuffFlowComponent.h"
#include "FlowAsset.h"

UBFNode_WaitGameplayEvent::UBFNode_WaitGameplayEvent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("BuffFlow|Tag");
#endif
	InputPins  = { FFlowPin(TEXT("In")), FFlowPin(TEXT("Stop")) };
	OutputPins = { FFlowPin(TEXT("Out")) };
}

FGameplayTag UBFNode_WaitGameplayEvent::GetRuntimeEventTag() const
{
	const UFlowAsset* FlowAsset = GetFlowAsset();
	const FString FlowPath = GetPathNameSafe(FlowAsset);
	if (FlowPath.Contains(TEXT("/DeathPoison/")))
	{
		return FGameplayTag::RequestGameplayTag(TEXT("Ability.Event.DeathAnimComplete"), false);
	}

	if (EventTag.IsValid())
	{
		return EventTag;
	}

	return EventTag;
}

void UBFNode_WaitGameplayEvent::ExecuteInput(const FName& PinName)
{
	const FGameplayTag RuntimeEventTag = GetRuntimeEventTag();
	if (PinName == TEXT("In"))
	{
		// 先解绑旧监听，防止重复绑定
		if (BoundASC.IsValid() && EventDelegateHandle.IsValid())
		{
			FGameplayTagContainer TagContainer;
			TagContainer.AddTag(RuntimeEventTag);
			BoundASC->RemoveGameplayEventTagContainerDelegate(TagContainer, EventDelegateHandle);
			EventDelegateHandle.Reset();
			BoundASC.Reset();
		}

		if (!RuntimeEventTag.IsValid())
		{
			UE_LOG(LogTemp, Warning, TEXT("[BFNode_WaitGameplayEvent] EventTag 无效，无法监听"));
			return;
		}

		AActor* TargetActor = ResolveTarget(Target);
		if (!TargetActor)
		{
			UE_LOG(LogTemp, Warning, TEXT("[BFNode_WaitGameplayEvent] Target 解析为 null，无法绑定监听"));
			return;
		}

		UAbilitySystemComponent* ASC = TargetActor->FindComponentByClass<UAbilitySystemComponent>();
		if (!ASC)
		{
			UE_LOG(LogTemp, Warning, TEXT("[BFNode_WaitGameplayEvent] Target(%s) 没有 ASC 组件"),
				*TargetActor->GetName());
			return;
		}

		FGameplayTagContainer TagContainer;
		TagContainer.AddTag(RuntimeEventTag);

		BoundASC = ASC;
		EventDelegateHandle = ASC->AddGameplayEventTagContainerDelegate(
			TagContainer,
			FGameplayEventTagMulticastDelegate::FDelegate::CreateUObject(
				this, &UBFNode_WaitGameplayEvent::HandleGameplayEvent));

		UE_LOG(LogTemp, Verbose, TEXT("[BFNode_WaitGameplayEvent] 开始监听 Tag=%s 在 Actor=%s"),
			*RuntimeEventTag.ToString(), *TargetActor->GetName());
	}
	else if (PinName == TEXT("Stop"))
	{
		Cleanup();
	}
}

void UBFNode_WaitGameplayEvent::HandleGameplayEvent(FGameplayTag Tag, const FGameplayEventData* Payload)
{
	// 将事件携带的 Instigator / Target 写入 EventContext，供下游节点读取
	if (UBuffFlowComponent* BFC = GetBuffFlowComponent())
	{
		BFC->LastEventContext.EventTag = Tag;
		if (Payload)
		{
			BFC->LastEventContext.DamageCauser   = const_cast<AActor*>(Payload->Instigator.Get());
			BFC->LastEventContext.DamageReceiver = const_cast<AActor*>(Payload->Target.Get());
			BFC->LastEventContext.DamageAmount   = Payload->EventMagnitude;

			EventMagnitude = FFlowDataPinOutputProperty_Float(Payload->EventMagnitude);
		}
	}

	// false = 不结束节点，持续监听
	TriggerOutput(TEXT("Out"), false);
}

void UBFNode_WaitGameplayEvent::Cleanup()
{
	if (BoundASC.IsValid() && EventDelegateHandle.IsValid())
	{
		const FGameplayTag RuntimeEventTag = GetRuntimeEventTag();
		FGameplayTagContainer TagContainer;
		TagContainer.AddTag(RuntimeEventTag);
		BoundASC->RemoveGameplayEventTagContainerDelegate(TagContainer, EventDelegateHandle);
		EventDelegateHandle.Reset();
	}
	BoundASC.Reset();

	Super::Cleanup();
}
