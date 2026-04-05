#include "BuffFlow/Nodes/BFNode_ApplyRuneGE.h"
#include "AbilitySystemComponent.h"
#include "Data/RuneDataAsset.h"
#include "GameplayEffect.h"

UBFNode_ApplyRuneGE::UBFNode_ApplyRuneGE(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("BuffFlow|Effect");
#endif
	OutputPins = { FFlowPin(TEXT("Out")), FFlowPin(TEXT("Failed")) };
}

void UBFNode_ApplyRuneGE::ExecuteInput(const FName& PinName)
{
	if (!RuneAsset)
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

	UAbilitySystemComponent* ASC = TargetActor->FindComponentByClass<UAbilitySystemComponent>();
	if (!ASC)
	{
		TriggerOutput(TEXT("Failed"), true);
		return;
	}

	// DA → TransientGE（包含 RuneConfig 的 Duration/Stack/UniqueType + Effects 的属性修改）
	UGameplayEffect* GE = RuneAsset->CreateTransientGE(GetTransientPackage());
	if (!GE)
	{
		TriggerOutput(TEXT("Failed"), true);
		return;
	}

	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	FGameplayEffectSpec Spec(GE, Context, static_cast<float>(Level));
	FActiveGameplayEffectHandle Handle = ASC->ApplyGameplayEffectSpecToSelf(Spec);

	if (!Handle.IsValid())
	{
		TriggerOutput(TEXT("Failed"), true);
		return;
	}

	// 首次施加时记录 handle + ASC（叠层时 GAS 内部聚合，handle 始终有效）
	if (!GrantedHandle.IsValid())
	{
		GrantedHandle = Handle;
		GrantedASC    = ASC;
	}

	TriggerOutput(TEXT("Out"), true);
}

void UBFNode_ApplyRuneGE::Cleanup()
{
	if (GrantedASC.IsValid() && RuneAsset)
	{
		// 通过 RuneTag 移除所有层（单 handle 移除只移除一层，tag 移除可清空叠层）
		const FGameplayTag& Tag = RuneAsset->RuneTemplate.RuneConfig.RuneTag;
		if (Tag.IsValid())
		{
			FGameplayTagContainer Tags;
			Tags.AddTag(Tag);
			GrantedASC->RemoveActiveEffectsWithTags(Tags);
		}
		else if (GrantedHandle.IsValid())
		{
			// 无 Tag 时退化为 handle 移除
			GrantedASC->RemoveActiveGameplayEffect(GrantedHandle);
		}
	}

	GrantedHandle = FActiveGameplayEffectHandle();
	GrantedASC.Reset();

	Super::Cleanup();
}
