#include "BuffFlow/Nodes/BFNode_SendGameplayEvent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffectTypes.h"

UBFNode_SendGameplayEvent::UBFNode_SendGameplayEvent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("BuffFlow|Effect");
#endif
	OutputPins = { FFlowPin(TEXT("Out")), FFlowPin(TEXT("Failed")) };
}

void UBFNode_SendGameplayEvent::ExecuteInput(const FName& PinName)
{
	if (!EventTag.IsValid())
	{
		TriggerOutput(TEXT("Failed"), true);
		return;
	}

	AActor* TargetActor = ResolveTarget(Target);
	if (!TargetActor)
	{
		TriggerOutput(TEXT("Failed"), true);
		return;
	}

	UAbilitySystemComponent* TargetASC = TargetActor->FindComponentByClass<UAbilitySystemComponent>();
	if (!TargetASC)
	{
		TriggerOutput(TEXT("Failed"), true);
		return;
	}

	// 从数据引脚（或属性默认值）读取 RuneAsset
	FFlowDataPinResult_Object RuneAssetResult = TryResolveDataPinAsObject(GET_MEMBER_NAME_CHECKED(UBFNode_SendGameplayEvent, RuneAsset));
	UObject* OptionalObj = (RuneAssetResult.Result == EFlowDataPinResolveResult::Success)
		? RuneAssetResult.Value.Get()
		: RuneAsset.GetObjectValue();

	FGameplayEventData EventData;
	EventData.EventTag = EventTag;
	EventData.EventMagnitude = EventMagnitude;
	EventData.OptionalObject = OptionalObj;
	EventData.Instigator = ResolveTarget(EBFTargetSelector::BuffOwner);
	EventData.Target = TargetActor;

	TargetASC->HandleGameplayEvent(EventTag, &EventData);

	TriggerOutput(TEXT("Out"), true);
}
