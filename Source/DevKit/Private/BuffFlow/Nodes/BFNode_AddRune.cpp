#include "BuffFlow/Nodes/BFNode_AddRune.h"
#include "BuffFlow/BuffFlowComponent.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "Character/YogCharacterBase.h"
#include "Data/RuneDataAsset.h"
#include "GameplayEffect.h"

UBFNode_AddRune::UBFNode_AddRune(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("BuffFlow|Effect");
#endif
	OutputPins = { FFlowPin(TEXT("Out")), FFlowPin(TEXT("Failed")) };
}

void UBFNode_AddRune::ExecuteInput(const FName& PinName)
{
	if (!RuneAsset)
	{
		TriggerOutput(TEXT("Failed"), true);
		return;
	}

	AActor* TargetActor = ResolveTarget(Target);
	UAbilitySystemComponent* ASC = nullptr;
	if (TargetActor)
	{
		ASC = TargetActor->FindComponentByClass<UAbilitySystemComponent>();
	}

	if (!ASC)
	{
		TriggerOutput(TEXT("Failed"), true);
		return;
	}

	// 1. 施加 GE
	UGameplayEffect* TransientGE = RuneAsset->CreateTransientGE(GetTransientPackage());
	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	FGameplayEffectSpec Spec(TransientGE, Context, static_cast<float>(Level));
	FActiveGameplayEffectHandle Handle = ASC->ApplyGameplayEffectSpecToSelf(Spec);

	if (!Handle.IsValid())
	{
		TriggerOutput(TEXT("Failed"), true);
		return;
	}

	// 2. 如果 RuneAsset 配置了 FlowAsset，在目标 BuffFlowComponent 上启动 Flow
	if (RuneAsset->RuneTemplate.Flow.BuffFlowAsset)
	{
		if (UBuffFlowComponent* TargetBFC = TargetActor->FindComponentByClass<UBuffFlowComponent>())
		{
			// 使用 DA 路径哈希生成确定性 Guid，使 RemoveRune 能精确停止对应 Flow
			const uint32 Hash = GetTypeHash(RuneAsset->GetPathName());
			FGuid RuneGuid(Hash, 0, 0, 0);
			AActor* Giver = GetBuffOwner();
			TargetBFC->StartBuffFlow(RuneAsset->RuneTemplate.Flow.BuffFlowAsset, RuneGuid, Giver);
		}
	}

	TriggerOutput(TEXT("Out"), true);
}
