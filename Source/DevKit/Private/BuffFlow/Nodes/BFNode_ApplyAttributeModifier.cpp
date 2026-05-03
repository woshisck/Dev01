#include "BuffFlow/Nodes/BFNode_ApplyAttributeModifier.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/Attribute/BaseAttributeSet.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "BuffFlow/BuffFlowComponent.h"
#include "Types/FlowDataPinResults.h"

UBFNode_ApplyAttributeModifier::UBFNode_ApplyAttributeModifier(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("BuffFlow|Effect");
#endif
	InputPins  = { FFlowPin(TEXT("In")), FFlowPin(TEXT("Remove")) };
	OutputPins = { FFlowPin(TEXT("Out")), FFlowPin(TEXT("Expired")), FFlowPin(TEXT("Failed")) };
}

void UBFNode_ApplyAttributeModifier::ExecuteInput(const FName& PinName)
{
	// Remove 引脚：主动移除当前持有的 GE（一次性 Buff 消耗用）
	if (PinName == TEXT("Remove"))
	{
		if (UWorld* World = GetWorld())
		{
			for (TPair<TObjectKey<UAbilitySystemComponent>, FRuntimeGrantState>& Pair : RuntimeGrantStates)
			{
				World->GetTimerManager().ClearTimer(Pair.Value.ExpiryTimer);
			}
		}

		for (TPair<TObjectKey<UAbilitySystemComponent>, FRuntimeGrantState>& Pair : RuntimeGrantStates)
		{
			UAbilitySystemComponent* StateASC = Pair.Key.ResolveObjectPtr();
			if (!StateASC)
			{
				continue;
			}

			if (Pair.Value.EffectHandle.IsValid())
			{
				StateASC->RemoveActiveGameplayEffect(Pair.Value.EffectHandle);
			}

			if (!GrantedTagsToASC.IsEmpty() && Pair.Value.bManualGrantActive)
			{
				StateASC->RemoveLooseGameplayTags(GrantedTagsToASC);
			}

			for (const FGameplayAbilitySpecHandle& AbilHandle : Pair.Value.AbilityHandles)
			{
				if (AbilHandle.IsValid())
				{
					StateASC->ClearAbility(AbilHandle);
				}
			}
		}

		RuntimeGrantStates.Reset();
		GrantedAbilityHandles.Empty();
		GrantedHandle = FActiveGameplayEffectHandle();
		GrantedASC.Reset();
		TriggerOutput(TEXT("Out"), false);
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("[ApplyAttrMod] ExecuteInput | Pin=%s | Attr=%s | GrantedTags=%s | GrantedAbilities=%d"),
		*PinName.ToString(),
		Attribute.IsValid() ? *Attribute.GetName() : TEXT("(none)"),
		*GrantedTagsToASC.ToStringSimple(),
		GrantedAbilities.Num());

	// Attribute、GrantedTagsToASC、GrantedAbilities 三者至少配置一个才有意义
	if (!Attribute.IsValid() && GrantedTagsToASC.IsEmpty() && GrantedAbilities.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("[ApplyAttrMod] FAILED: Nothing configured (Attribute / GrantedTagsToASC / GrantedAbilities are all empty)"));
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

	UE_LOG(LogTemp, Warning, TEXT("[ApplyAttrMod] Target=%s"), *TargetActor->GetName());

	UAbilitySystemComponent* ASC = TargetActor->FindComponentByClass<UAbilitySystemComponent>();
	if (!ASC)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ApplyAttrMod] FAILED: No ASC found on %s"), *TargetActor->GetName());
		TriggerOutput(TEXT("Failed"), true);
		return;
	}


	// Value：优先读取连入的数据引脚，无连线则使用节点上的固定值（Attribute 无效时忽略）
	const float AttributeBeforeApply = Attribute.IsValid()
		? ASC->GetNumericAttribute(Attribute)
		: 0.f;

	float ResolvedValue = Value.Value;
	if (Attribute.IsValid())
	{
		FFlowDataPinResult_Float PinResult = TryResolveDataPinAsFloat(
			GET_MEMBER_NAME_CHECKED(UBFNode_ApplyAttributeModifier, Value));
		if (PinResult.Result == EFlowDataPinResolveResult::Success)
		{
			ResolvedValue = PinResult.Value;
		}
	}

	if (bUseCombatCardEffectMultiplier)
	{
		if (UBuffFlowComponent* BFC = GetBuffFlowComponent())
		{
			if (BFC->HasCombatCardEffectContext())
			{
				const FCombatCardEffectContext& CombatCardContext = BFC->GetLastCombatCardEffectContext();
				ResolvedValue *= FMath::Max(0.0f, CombatCardContext.EffectMultiplier);
			}
		}
	}

	if (Attribute == UBaseAttributeSet::GetArmorHPAttribute())
	{
		if (const UBaseAttributeSet* BaseSet = ASC->GetSet<UBaseAttributeSet>())
		{
			const float CurrentArmor = BaseSet->GetArmorHP();
			const float CurrentMaxArmor = BaseSet->GetMaxArmorHP();
			float ExpectedArmor = CurrentArmor;

			switch (ModOp)
			{
			case EGameplayModOp::Additive:
				ExpectedArmor = DurationType == ERuneDurationType::Instant
					? CurrentArmor + ResolvedValue
					: FMath::Max(CurrentArmor, ResolvedValue);
				break;
			case EGameplayModOp::Multiplicitive:
				ExpectedArmor = CurrentArmor * ResolvedValue;
				break;
			case EGameplayModOp::Override:
				ExpectedArmor = ResolvedValue;
				break;
			default:
				break;
			}

			if (ExpectedArmor > CurrentMaxArmor)
			{
				const_cast<UBaseAttributeSet*>(BaseSet)->SetMaxArmorHP(ExpectedArmor);
				UE_LOG(LogTemp, Warning, TEXT("[EnemyRune][Armor] Raise MaxArmorHP Target=%s %.1f -> %.1f before ArmorHP mod %.1f"),
					*GetNameSafe(TargetActor),
					CurrentMaxArmor,
					ExpectedArmor,
					ResolvedValue);
			}

			if (DurationType != ERuneDurationType::Instant && GrantedTagsToASC.IsEmpty() && GrantedAbilities.IsEmpty())
			{
				ASC->SetNumericAttributeBase(UBaseAttributeSet::GetMaxArmorHPAttribute(), FMath::Max(CurrentMaxArmor, ExpectedArmor));
				ASC->SetNumericAttributeBase(UBaseAttributeSet::GetArmorHPAttribute(), ExpectedArmor);
				UE_LOG(LogTemp, Warning, TEXT("[EnemyRune][Armor] DirectGrant Target=%s ArmorHP=%.1f MaxArmorHP=%.1f"),
					*GetNameSafe(TargetActor),
					ExpectedArmor,
					FMath::Max(CurrentMaxArmor, ExpectedArmor));
				TriggerOutput(TEXT("Out"), false);
				return;
			}
		}
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

	// Attribute Modifier（可选，Attribute 无效时跳过）
	CachedGE->Modifiers.Reset();
	if (Attribute.IsValid())
	{
		FGameplayModifierInfo ModInfo;
		ModInfo.Attribute         = Attribute;
		ModInfo.ModifierOp        = ModOp;
		ModInfo.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(ResolvedValue));
		CachedGE->Modifiers.Add(ModInfo);
	}

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
	const float AttributeAfterApply = Attribute.IsValid()
		? ASC->GetNumericAttribute(Attribute)
		: 0.f;

	UE_LOG(LogTemp, Warning, TEXT("[ApplyAttrMod] ApplyGE Handle.IsValid=%d Target=%s Attr=%s Before=%.2f Value=%.2f After=%.2f DurationType=%d ModOp=%d GrantedTags=%s GrantedAbilities=%d"),
		(int32)Handle.IsValid(),
		*TargetActor->GetName(),
		Attribute.IsValid() ? *Attribute.GetName() : TEXT("(none)"),
		AttributeBeforeApply,
		ResolvedValue,
		AttributeAfterApply,
		static_cast<int32>(DurationType),
		static_cast<int32>(ModOp.GetValue()),
		*GrantedTagsToASC.ToStringSimple(),
		GrantedAbilities.Num());

	// Instant GE 应用后 handle 即失效，属正常现象；非瞬发 GE handle 无效视为失败
	if (DurationType != ERuneDurationType::Instant && !Handle.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("[ApplyAttrMod] FAILED: GE handle invalid after Apply (likely blocked by tags or requirements)"));
		TriggerOutput(TEXT("Failed"), true);
		return;
	}

	// 存储 handle 供 Cleanup 使用（Instant 无需存储）
	TObjectKey<UAbilitySystemComponent> ASCKey(ASC);
	FRuntimeGrantState* GrantState = nullptr;
	if (DurationType != ERuneDurationType::Instant)
	{
		GrantState = &RuntimeGrantStates.FindOrAdd(ASCKey);
		GrantState->EffectHandle = Handle;
	}

	if (DurationType != ERuneDurationType::Instant)
	{
		GrantedHandle = Handle;
		GrantedASC    = ASC;
	}

	// 手动授予 GrantedTagsToASC 和 GrantedAbilities
	// UE5.4 对动态 NewObject GE 不支持 GE.GrantedAbilities / InheritableOwnedTagsContainer，
	// 改为手动 AddLooseGameplayTags + GiveAbility，由 ExpiryTimer / Cleanup 负责撤销。
	// 仅首次施加时授予（同一目标 ASC 且已有 handle = 堆叠刷新，跳过重复授予）。
	const bool bFirstGrant = GrantState && !GrantState->bManualGrantActive;
	if (bFirstGrant && DurationType != ERuneDurationType::Instant)
	{
		// 必须先 GiveAbility，再 AddLooseGameplayTags！
		// OwnedTagPresent 触发器在 Tag 加入时扫描 ASC 已有的 AbilitySpec，
		// 若 GA 尚未 Grant，Tag 加入时找不到触发器，GA 永远不会激活。
		GrantedAbilityHandles.Empty();
		GrantState->AbilityHandles.Empty();
		for (const TSubclassOf<UGameplayAbility>& AbilityClass : GrantedAbilities)
		{
			if (AbilityClass)
			{
				FGameplayAbilitySpecHandle AbilHandle = ASC->GiveAbility(FGameplayAbilitySpec(AbilityClass, 1));
				if (AbilHandle.IsValid())
				{
					GrantedAbilityHandles.Add(AbilHandle);
					GrantState->AbilityHandles.Add(AbilHandle);
					UE_LOG(LogTemp, Warning, TEXT("[ApplyAttrMod] GiveAbility=%s → %s"),
						*AbilityClass->GetName(), *TargetActor->GetName());
				}
			}
		}

		// GA 已在 ASC，现在加 Tag → 触发 OwnedTagPresent → GA 激活
		if (!GrantedTagsToASC.IsEmpty())
		{
			ASC->AddLooseGameplayTags(GrantedTagsToASC);
			UE_LOG(LogTemp, Warning, TEXT("[ApplyAttrMod] AddLooseGameplayTags=%s → %s"),
				*GrantedTagsToASC.ToStringSimple(), *TargetActor->GetName());
		}
		GrantState->bManualGrantActive = true;
	}

	// HasDuration 时启动过期计时器，到期触发 Expired 引脚并撤销手动授予的 Tag/GA
	// 每次 In 触发（含堆叠刷新）都重置倒计时，与 GAS GE 的实际剩余时间保持同步
	if (DurationType == ERuneDurationType::Duration && Duration > 0.f)
	{
		if (UWorld* World = GetWorld())
		{
			TWeakObjectPtr<UAbilitySystemComponent> WeakASC(ASC);
			TWeakObjectPtr<UBFNode_ApplyAttributeModifier> WeakThis(this);
			FTimerHandle& TargetExpiryTimer = GrantState ? GrantState->ExpiryTimer : ExpiryTimer;
			World->GetTimerManager().ClearTimer(TargetExpiryTimer);
			World->GetTimerManager().SetTimer(TargetExpiryTimer, [WeakThis, WeakASC, ASCKey]()
			{
				UBFNode_ApplyAttributeModifier* Node = WeakThis.Get();
				if (!Node)
				{
					return;
				}

				FRuntimeGrantState* State = Node->RuntimeGrantStates.Find(ASCKey);
				if (WeakASC.IsValid() && State)
				{
					if (!Node->GrantedTagsToASC.IsEmpty() && State->bManualGrantActive)
					{
						WeakASC->RemoveLooseGameplayTags(Node->GrantedTagsToASC);
					}
					for (const FGameplayAbilitySpecHandle& AbilHandle : State->AbilityHandles)
					{
						if (AbilHandle.IsValid())
						{
							WeakASC->ClearAbility(AbilHandle);
						}
					}
				}
				Node->RuntimeGrantStates.Remove(ASCKey);
				Node->TriggerOutput(TEXT("Expired"), true);
			}, Duration, false);
		}
	}

	// bFinish=false：让节点保持活跃，Cleanup() 在 StopBuffFlow 时才调用（届时移除 GE）
	TriggerOutput(TEXT("Out"), false);
}

void UBFNode_ApplyAttributeModifier::Cleanup()
{
	// FA 提前停止时取消计时器（避免 Expired 在 Cleanup 后错误触发）
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ExpiryTimer);
		for (TPair<TObjectKey<UAbilitySystemComponent>, FRuntimeGrantState>& Pair : RuntimeGrantStates)
		{
			World->GetTimerManager().ClearTimer(Pair.Value.ExpiryTimer);
		}
	}

	for (TPair<TObjectKey<UAbilitySystemComponent>, FRuntimeGrantState>& Pair : RuntimeGrantStates)
	{
		UAbilitySystemComponent* StateASC = Pair.Key.ResolveObjectPtr();
		if (!StateASC)
		{
			continue;
		}

		// 移除 GE
		if (Pair.Value.EffectHandle.IsValid())
			StateASC->RemoveActiveGameplayEffect(Pair.Value.EffectHandle);

		// 撤销手动授予的 Tag
		if (!GrantedTagsToASC.IsEmpty() && Pair.Value.bManualGrantActive)
			StateASC->RemoveLooseGameplayTags(GrantedTagsToASC);

		// 撤销手动授予的 GA
		for (const FGameplayAbilitySpecHandle& AbilHandle : Pair.Value.AbilityHandles)
		{
			if (AbilHandle.IsValid())
				StateASC->ClearAbility(AbilHandle);
		}
	}

	RuntimeGrantStates.Reset();
	GrantedAbilityHandles.Empty();
	GrantedHandle = FActiveGameplayEffectHandle();
	GrantedASC.Reset();
	CachedGE = nullptr;

	Super::Cleanup();
}
