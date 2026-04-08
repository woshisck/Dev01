#include "BuffFlow/Nodes/BFNode_ApplyModifierDA.h"
#include "Data/GEConfigDataAsset.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "Types/FlowDataPinResults.h"

UBFNode_ApplyModifierDA::UBFNode_ApplyModifierDA(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("BuffFlow|Effect");
#endif
	OutputPins = { FFlowPin(TEXT("Out")), FFlowPin(TEXT("Failed")) };
}

void UBFNode_ApplyModifierDA::ExecuteInput(const FName& PinName)
{
	if (!Config)
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

	// Value：数据引脚连线时覆盖，否则使用 DA 默认值
	float ResolvedValue = Config->DefaultValue;
	FFlowDataPinResult_Float PinResult = TryResolveDataPinAsFloat(
		GET_MEMBER_NAME_CHECKED(UBFNode_ApplyModifierDA, Value));
	if (PinResult.Result == EFlowDataPinResolveResult::Success)
	{
		ResolvedValue = PinResult.Value;
	}

	// 复用同一 GE 对象（GAS 堆叠依赖相同 Def 指针，每次 NewObject 会使堆叠失效）
	if (!CachedGE)
	{
		CachedGE = NewObject<UGameplayEffect>(GetTransientPackage(), NAME_None, RF_Transient);
	}

	// 从 DA 读取并更新配置（每次都更新以支持热修改场景）
	switch (Config->DurationType)
	{
	case ERuneDurationType::Instant:
		CachedGE->DurationPolicy = EGameplayEffectDurationType::Instant;
		break;
	case ERuneDurationType::Infinite:
		CachedGE->DurationPolicy = EGameplayEffectDurationType::Infinite;
		break;
	case ERuneDurationType::Duration:
		CachedGE->DurationPolicy = EGameplayEffectDurationType::HasDuration;
		CachedGE->DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(Config->Duration));
		break;
	}

	switch (Config->StackMode)
	{
	case EBFGEStackMode::None:
		CachedGE->StackingType    = EGameplayEffectStackingType::None;
		CachedGE->StackLimitCount = 0;
		break;
	case EBFGEStackMode::Unique:
		CachedGE->StackingType    = EGameplayEffectStackingType::AggregateByTarget;
		CachedGE->StackLimitCount = 1;
		break;
	case EBFGEStackMode::Stackable:
		CachedGE->StackingType    = EGameplayEffectStackingType::AggregateByTarget;
		CachedGE->StackLimitCount = Config->StackLimitCount;
		break;
	}
	CachedGE->StackDurationRefreshPolicy = Config->StackDurationRefreshPolicy;
	CachedGE->StackExpirationPolicy      = Config->StackExpirationPolicy;

	CachedGE->Modifiers.Reset();
	FGameplayModifierInfo ModInfo;
	ModInfo.Attribute         = Config->Attribute;
	ModInfo.ModifierOp        = Config->ModOp;
	ModInfo.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(ResolvedValue));
	CachedGE->Modifiers.Add(ModInfo);

	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	FGameplayEffectSpec Spec(CachedGE, Context, 1.f);

	if (Config->DynamicAssetTags.IsValid())
		Spec.AppendDynamicAssetTags(Config->DynamicAssetTags);

	// 透传 Owner Tags
	if (!Config->PassThroughOwnerTags.IsEmpty())
	{
		if (UYogAbilitySystemComponent* OwnerASC = GetOwnerASC())
		{
			for (const FGameplayTag& Tag : Config->PassThroughOwnerTags)
			{
				if (OwnerASC->HasMatchingGameplayTag(Tag))
					Spec.AddDynamicAssetTag(Tag);
			}
		}
	}

	FActiveGameplayEffectHandle Handle = ASC->ApplyGameplayEffectSpecToSelf(Spec);

	// Instant GE 应用后 handle 即失效，属正常现象；非瞬发 GE handle 无效视为失败
	if (Config->DurationType != ERuneDurationType::Instant && !Handle.IsValid())
	{
		TriggerOutput(TEXT("Failed"), true);
		return;
	}

	if (Config->DurationType != ERuneDurationType::Instant)
	{
		GrantedHandle = Handle;
		GrantedASC    = ASC;
	}

	// bFinish=false：保持节点活跃，Cleanup() 在 FA 停止时移除 GE
	TriggerOutput(TEXT("Out"), false);
}

void UBFNode_ApplyModifierDA::Cleanup()
{
	if (GrantedASC.IsValid() && GrantedHandle.IsValid())
	{
		GrantedASC->RemoveActiveGameplayEffect(GrantedHandle);
	}

	GrantedHandle = FActiveGameplayEffectHandle();
	GrantedASC.Reset();
	CachedGE = nullptr;

	Super::Cleanup();
}
