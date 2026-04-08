#include "BuffFlow/Nodes/BFNode_ApplyAttributeModifier.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "Types/FlowDataPinResults.h"

UBFNode_ApplyAttributeModifier::UBFNode_ApplyAttributeModifier(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("BuffFlow|Effect");
#endif
	InputPins  = { FFlowPin(TEXT("In")), FFlowPin(TEXT("Remove")) };
	OutputPins = { FFlowPin(TEXT("Out")), FFlowPin(TEXT("Failed")) };
}

void UBFNode_ApplyAttributeModifier::ExecuteInput(const FName& PinName)
{
	// Remove 引脚：主动移除当前持有的 GE（一次性 Buff 消耗用）
	if (PinName == TEXT("Remove"))
	{
		if (GrantedASC.IsValid() && GrantedHandle.IsValid())
		{
			GrantedASC->RemoveActiveGameplayEffect(GrantedHandle);
			GrantedHandle = FActiveGameplayEffectHandle();
			GrantedASC.Reset();
		}
		TriggerOutput(TEXT("Out"), false);
		return;
	}

	if (!Attribute.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("[ApplyAttrMod] FAILED: Attribute is invalid"));
		TriggerOutput(TEXT("Failed"), true);
		return;
	}

	AActor* TargetActor = ResolveTarget(Target);
	if (!TargetActor)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ApplyAttrMod] FAILED: ResolveTarget returned null (Target=%d)"), (int32)Target);
		TriggerOutput(TEXT("Failed"), true);
		return;
	}

	UAbilitySystemComponent* ASC = TargetActor->FindComponentByClass<UAbilitySystemComponent>();
	if (!ASC)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ApplyAttrMod] FAILED: No ASC found on %s"), *TargetActor->GetName());
		TriggerOutput(TEXT("Failed"), true);
		return;
	}


	// Value：优先读取连入的数据引脚，无连线则使用节点上的固定值
	float ResolvedValue = Value.Value;
	FFlowDataPinResult_Float PinResult = TryResolveDataPinAsFloat(
		GET_MEMBER_NAME_CHECKED(UBFNode_ApplyAttributeModifier, Value));
	if (PinResult.Result == EFlowDataPinResolveResult::Success)
	{
		ResolvedValue = PinResult.Value;
	}

	// 复用同一个 GE 对象（GAS 堆叠规则依赖相同的 Def 指针，每次 NewObject 会导致堆叠失效）
	if (!CachedGE)
	{
		CachedGE = NewObject<UGameplayEffect>(GetTransientPackage(), NAME_None, RF_Transient);
	}

	// 每次都更新配置（支持数据引脚动态值）
	switch (DurationType)
	{
	case ERuneDurationType::Instant:
		CachedGE->DurationPolicy = EGameplayEffectDurationType::Instant;
		break;
	case ERuneDurationType::Infinite:
		CachedGE->DurationPolicy = EGameplayEffectDurationType::Infinite;
		break;
	case ERuneDurationType::Duration:
		CachedGE->DurationPolicy = EGameplayEffectDurationType::HasDuration;
		CachedGE->DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(Duration));
		break;
	}

	// Period 支持：> 0 时让 GAS 每隔 N 秒执行一次 Modifier
	// 典型用途：每秒 +1 热度（Period=1.0, DurationType=Infinite）
	if (Period > 0.f && DurationType != ERuneDurationType::Instant)
	{
		CachedGE->Period = FScalableFloat(Period);
		CachedGE->bExecutePeriodicEffectOnApplication = bFireImmediately;
	}
	else
	{
		CachedGE->Period = FScalableFloat(0.f);
	}

	switch (StackMode)
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
		CachedGE->StackLimitCount = StackLimitCount;
		break;
	}
	CachedGE->StackDurationRefreshPolicy = StackDurationRefreshPolicy;
	CachedGE->StackExpirationPolicy      = StackExpirationPolicy;

	CachedGE->Modifiers.Reset();
	FGameplayModifierInfo ModInfo;
	ModInfo.Attribute         = Attribute;
	ModInfo.ModifierOp        = ModOp;
	ModInfo.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(ResolvedValue));
	CachedGE->Modifiers.Add(ModInfo);

	UGameplayEffect* GE = CachedGE;

	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	FGameplayEffectSpec Spec(GE, Context, 1.f);
	if (DynamicAssetTags.IsValid())
		Spec.AppendDynamicAssetTags(DynamicAssetTags);

	// PassThroughOwnerTags：把 BuffOwner 身上已有的指定 Tag 透传到 Spec
	if (!PassThroughOwnerTags.IsEmpty())
	{
		if (UYogAbilitySystemComponent* OwnerASC = GetOwnerASC())
		{
			for (const FGameplayTag& Tag : PassThroughOwnerTags)
			{
				if (OwnerASC->HasMatchingGameplayTag(Tag))
					Spec.AddDynamicAssetTag(Tag);
			}
		}
	}

	FActiveGameplayEffectHandle Handle = ASC->ApplyGameplayEffectSpecToSelf(Spec);


	// Instant GE 应用后 handle 即失效，属正常现象；非瞬发 GE handle 无效视为失败
	if (DurationType != ERuneDurationType::Instant && !Handle.IsValid())
	{
		TriggerOutput(TEXT("Failed"), true);
		return;
	}

	// 存储 handle 供 Cleanup 使用（Instant 无需存储）
	if (DurationType != ERuneDurationType::Instant)
	{
		GrantedHandle = Handle;
		GrantedASC    = ASC;
	}

	// bFinish=false：让节点保持活跃，Cleanup() 在 StopBuffFlow 时才调用（届时移除 GE）
	TriggerOutput(TEXT("Out"), false);
}

void UBFNode_ApplyAttributeModifier::Cleanup()
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
