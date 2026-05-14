#include "BuffFlow/Nodes/BFNode_OnTagAdded.h"
#include "AbilitySystemComponent.h"

UBFNode_OnTagAdded::UBFNode_OnTagAdded(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("BuffFlow|Tag");
#endif
	InputPins  = { FFlowPin(TEXT("In")), FFlowPin(TEXT("Stop")) };
	OutputPins = { FFlowPin(TEXT("Out")) };
}

void UBFNode_OnTagAdded::ExecuteBuffFlowInput(const FName& PinName)
{
	if (PinName == TEXT("In"))
	{
		Unbind();

		if (!Tag.IsValid())
		{
			UE_LOG(LogTemp, Warning, TEXT("[BFNode_OnTagAdded] Tag 无效，无法监听"));
			return;
		}

		AActor* TargetActor = ResolveTarget(Target);
		if (!TargetActor)
		{
			UE_LOG(LogTemp, Warning, TEXT("[BFNode_OnTagAdded] Target 解析为 null"));
			return;
		}

		UAbilitySystemComponent* ASC = TargetActor->FindComponentByClass<UAbilitySystemComponent>();
		if (!ASC)
		{
			UE_LOG(LogTemp, Warning, TEXT("[BFNode_OnTagAdded] Target(%s) 没有 ASC"), *TargetActor->GetName());
			return;
		}

		BoundASC = ASC;
		TagDelegateHandle = ASC->RegisterGameplayTagEvent(Tag, EGameplayTagEventType::NewOrRemoved)
			.AddUObject(this, &UBFNode_OnTagAdded::HandleTagChanged);
	}
	else if (PinName == TEXT("Stop"))
	{
		Unbind();
	}
}

void UBFNode_OnTagAdded::HandleTagChanged(const FGameplayTag InTag, int32 NewCount)
{
	if (NewCount > 0)
		TriggerOutput(TEXT("Out"), false);
}

void UBFNode_OnTagAdded::Unbind()
{
	if (BoundASC.IsValid() && TagDelegateHandle.IsValid())
	{
		BoundASC->UnregisterGameplayTagEvent(TagDelegateHandle, Tag, EGameplayTagEventType::NewOrRemoved);
		TagDelegateHandle.Reset();
	}
	BoundASC.Reset();
}

void UBFNode_OnTagAdded::Cleanup()
{
	Unbind();
	Super::Cleanup();
}
