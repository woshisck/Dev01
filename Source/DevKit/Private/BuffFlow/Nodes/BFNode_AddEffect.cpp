#include "BuffFlow/Nodes/BFNode_AddEffect.h"
#include "BuffFlow/BuffFlowComponent.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "Character/YogCharacterBase.h"
#include "System/YogInstanceSubSystem.h"
#include "Data/EffectRegistry.h"
#include "Data/YogBuffDefinition.h"
#include "GameplayEffect.h"

UBFNode_AddEffect::UBFNode_AddEffect(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("BuffFlow|Effect");
#endif
	OutputPins = { FFlowPin(TEXT("Out")), FFlowPin(TEXT("Failed")) };
}

void UBFNode_AddEffect::ExecuteInput(const FName& PinName)
{
	if (!EffectTag.IsValid())
	{
		TriggerOutput(TEXT("Failed"), true);
		return;
	}

	UYogInstanceSubSystem* Subsystem = UYogInstanceSubSystem::Get(this);
	UEffectRegistry* Registry = Subsystem ? Subsystem->GetEffectRegistry() : nullptr;
	if (!Registry)
	{
		UE_LOG(LogTemp, Warning, TEXT("BFNode_AddEffect: EffectRegistry not configured in Project Settings → Yog"));
		TriggerOutput(TEXT("Failed"), true);
		return;
	}

	UYogBuffDefinition* BuffDef = Registry->FindEffect(EffectTag);
	if (!BuffDef)
	{
		UE_LOG(LogTemp, Warning, TEXT("BFNode_AddEffect: Tag [%s] not found in EffectRegistry"), *EffectTag.ToString());
		TriggerOutput(TEXT("Failed"), true);
		return;
	}

	// 2. 找目标 ASC
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

	// 3. 施加 GE
	UGameplayEffect* TransientGE = BuffDef->CreateTransientGE(GetTransientPackage());
	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	FGameplayEffectSpec Spec(TransientGE, Context, static_cast<float>(Level));
	FActiveGameplayEffectHandle Handle = ASC->ApplyGameplayEffectSpecToSelf(Spec);

	if (!Handle.IsValid())
	{
		TriggerOutput(TEXT("Failed"), true);
		return;
	}

	// 4. 如果 BuffDef 配置了 FlowAsset，启动目标 BFC 上的 Flow
	if (BuffDef->BuffFlowAsset)
	{
		if (UBuffFlowComponent* TargetBFC = TargetActor->FindComponentByClass<UBuffFlowComponent>())
		{
			const uint32 Hash = GetTypeHash(BuffDef->GetPathName());
			FGuid RuneGuid(Hash, 0, 0, 0);
			AActor* Giver = GetBuffOwner();
			TargetBFC->StartBuffFlow(BuffDef->BuffFlowAsset, RuneGuid, Giver);
		}
	}

	TriggerOutput(TEXT("Out"), true);
}
