#include "BuffFlow/Nodes/BFNode_RemoveRune.h"
#include "BuffFlow/BuffFlowComponent.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "Data/RuneDataAsset.h"
#include "AbilitySystemComponent.h"

UBFNode_RemoveRune::UBFNode_RemoveRune(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("BuffFlow|Effect");
#endif
	OutputPins = { FFlowPin(TEXT("Out")) };
}

void UBFNode_RemoveRune::ExecuteInput(const FName& PinName)
{
	if (!RuneAsset)
	{
		TriggerOutput(TEXT("Out"), true);
		return;
	}

	AActor* TargetActor = ResolveTarget(Target);
	UAbilitySystemComponent* ASC = nullptr;
	if (TargetActor)
	{
		ASC = TargetActor->FindComponentByClass<UAbilitySystemComponent>();
	}

	// 1. 通过 BuffTag 移除所有匹配 GE
	if (ASC && RuneAsset->RuneTemplate.RuneConfig.BuffTag.IsValid())
	{
		FGameplayTagContainer RemoveTags;
		RemoveTags.AddTag(RuneAsset->RuneTemplate.RuneConfig.BuffTag);
		ASC->RemoveActiveEffectsWithTags(RemoveTags);
	}

	// 2. 停止关联的 BuffFlow（与 AddRune 使用相同的确定性 Guid）
	if (RuneAsset->RuneTemplate.Flow.BuffFlowAsset && TargetActor)
	{
		if (UBuffFlowComponent* TargetBFC = TargetActor->FindComponentByClass<UBuffFlowComponent>())
		{
			const uint32 Hash = GetTypeHash(RuneAsset->GetPathName());
			FGuid RuneGuid(Hash, 0, 0, 0);
			TargetBFC->StopBuffFlow(RuneGuid);
		}
	}

	TriggerOutput(TEXT("Out"), true);
}
