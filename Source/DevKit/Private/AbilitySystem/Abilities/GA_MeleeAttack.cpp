#include "AbilitySystem/Abilities/GA_MeleeAttack.h"
#include "AbilitySystem/AbilityTask/YogAbilityTask_PlayMontageAndWaitForEvent.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "Character/YogCharacterBase.h"
#include "Component/CharacterDataComponent.h"
#include "Data/CharacterData.h"
#include "Data/AbilityData.h"

UGA_MeleeAttack::UGA_MeleeAttack()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

FActionData UGA_MeleeAttack::GetAbilityActionData_Implementation() const
{
	AYogCharacterBase* Owner = Cast<AYogCharacterBase>(GetOwningActorFromActorInfo());
	if (!Owner) return FActionData();

	UCharacterDataComponent* CDC = Owner->GetCharacterDataComponent();
	if (!CDC) return FActionData();

	UCharacterData* CD = CDC->GetCharacterData();
	if (!CD || !CD->AbilityData) return FActionData();

	// 用 AbilityTags 第一个 Tag 作为 AbilityMap 查找 Key
	FGameplayTag FirstTag;
	for (const FGameplayTag& Tag : AbilityTags)
	{
		FirstTag = Tag;
		break;
	}

	if (!FirstTag.IsValid()) return FActionData();

	const FActionData* Found = CD->AbilityData->AbilityMap.Find(FirstTag);
	return Found ? *Found : FActionData();
}

void UGA_MeleeAttack::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	UE_LOG(LogTemp, Warning, TEXT("[GA_MeleeAttack] ActivateAbility: %s"), *GetName());

	// 玩家 GA：检查消耗/冷却
	if (bRequireCommit && !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 提前读取 ActionData，SetBeforeATK GE 的 SetByCaller 值来自此处
	const FActionData ActionData = GetAbilityActionData();

	// 施加攻击前摇 GE（玩家 GA 配置，敌人 GA 留空跳过）
	// SetByCaller 值对应 AbilityData 该行的 Act* 字段
	if (StatBeforeATKEffect)
	{
		if (UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get())
		{
			static const FGameplayTag TAG_ActDamage    = FGameplayTag::RequestGameplayTag("Attribute.ActDamage");
			static const FGameplayTag TAG_ActRange     = FGameplayTag::RequestGameplayTag("Attribute.ActRange");
			static const FGameplayTag TAG_ActRes       = FGameplayTag::RequestGameplayTag("Attribute.ActResilience");
			static const FGameplayTag TAG_ActDmgReduce = FGameplayTag::RequestGameplayTag("Attribute.ActDmgReduce");

			FGameplayEffectContextHandle ContextHandle = ASC->MakeEffectContext();
			ContextHandle.AddSourceObject(this);
			FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(StatBeforeATKEffect, GetAbilityLevel(), ContextHandle);
			if (SpecHandle.IsValid())
			{
				SpecHandle.Data->SetSetByCallerMagnitude(TAG_ActDamage,    ActionData.ActDamage);
				SpecHandle.Data->SetSetByCallerMagnitude(TAG_ActRange,     ActionData.ActRange);
				SpecHandle.Data->SetSetByCallerMagnitude(TAG_ActRes,       ActionData.ActResilience);
				SpecHandle.Data->SetSetByCallerMagnitude(TAG_ActDmgReduce, ActionData.ActDmgReduce);
				StatBeforeATKHandle = ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
			}
		}
	}

	// 重置命中标志（为本次攻击的第一节做准备）
	if (AYogCharacterBase* Owner = Cast<AYogCharacterBase>(GetOwningActorFromActorInfo()))
	{
		Owner->bComboHitConnected = false;
	}

	// 蒙太奇从已读取的 ActionData 取
	UAnimMontage* Montage = ActionData.Montage;

	if (!Montage)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GA_MeleeAttack] No Montage found for %s — ability ended immediately."), *GetName());
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	// 监听 AnimNotify 伤害事件：必须明确传入 Tag，空容器不会注册任何监听
	FGameplayTagContainer DamageEventTags;
	DamageEventTags.AddTag(FGameplayTag::RequestGameplayTag(FName("GameplayEffect.DamageType.GeneralAttack")));

	// 创建复合任务：播放蒙太奇 + 监听 GameplayEvent（AnimNotify 触发伤害事件）
	UYogAbilityTask_PlayMontageAndWaitForEvent* Task =
		UYogAbilityTask_PlayMontageAndWaitForEvent::PlayMontageAndWaitForEvent(
			this,
			NAME_None,
			Montage,
			DamageEventTags,
			1.0f,
			NAME_None,
			true,   // bStopWhenAbilityEnds
			1.0f);

	Task->OnCompleted.AddDynamic(this, &UGA_MeleeAttack::OnMontageCompleted);
	Task->OnBlendOut.AddDynamic(this, &UGA_MeleeAttack::OnMontageBlendOut);
	Task->OnInterrupted.AddDynamic(this, &UGA_MeleeAttack::OnMontageInterrupted);
	Task->OnCancelled.AddDynamic(this, &UGA_MeleeAttack::OnMontageCancelled);
	Task->EventReceived.AddDynamic(this, &UGA_MeleeAttack::OnEventReceived);

	Task->ReadyForActivation();
}

void UGA_MeleeAttack::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;

	// 安全清理：技能结束时清空未消费的附加 Effect（蒙太奇被打断未触发 OnEventReceived 时保护用）
	if (AYogCharacterBase* Owner = Cast<AYogCharacterBase>(GetOwningActorFromActorInfo()))
	{
		Owner->PendingAdditionalHitEffects.Empty();
	}

	// 移除攻击前摇 GE
	if (StatBeforeATKHandle.IsValid())
	{
		if (ASC) ASC->RemoveActiveGameplayEffect(StatBeforeATKHandle);
		StatBeforeATKHandle = FActiveGameplayEffectHandle();
	}

	// 施加攻击后摇 GE（仅正常结束时，Cancel/Interrupt 不触发）
	if (!bWasCancelled && StatAfterATKEffect && ASC)
	{
		static const FGameplayTag TAG_ActDamage    = FGameplayTag::RequestGameplayTag("Attribute.ActDamage");
		static const FGameplayTag TAG_ActRange     = FGameplayTag::RequestGameplayTag("Attribute.ActRange");
		static const FGameplayTag TAG_ActRes       = FGameplayTag::RequestGameplayTag("Attribute.ActResilience");
		static const FGameplayTag TAG_ActDmgReduce = FGameplayTag::RequestGameplayTag("Attribute.ActDmgReduce");

		const FActionData AfterActionData = GetAbilityActionData();
		FGameplayEffectContextHandle Ctx = ASC->MakeEffectContext();
		Ctx.AddSourceObject(this);
		FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(StatAfterATKEffect, GetAbilityLevel(), Ctx);
		if (Spec.IsValid())
		{
			Spec.Data->SetSetByCallerMagnitude(TAG_ActDamage,    AfterActionData.ActDamage);
			Spec.Data->SetSetByCallerMagnitude(TAG_ActRange,     AfterActionData.ActRange);
			Spec.Data->SetSetByCallerMagnitude(TAG_ActRes,       AfterActionData.ActResilience);
			Spec.Data->SetSetByCallerMagnitude(TAG_ActDmgReduce, AfterActionData.ActDmgReduce);
			ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
		}
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_MeleeAttack::OnMontageCompleted(FGameplayTag EventTag, FGameplayEventData EventData)
{
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
}

void UGA_MeleeAttack::OnMontageBlendOut(FGameplayTag EventTag, FGameplayEventData EventData)
{
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
}

void UGA_MeleeAttack::OnMontageInterrupted(FGameplayTag EventTag, FGameplayEventData EventData)
{
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, true);
}

void UGA_MeleeAttack::OnMontageCancelled(FGameplayTag EventTag, FGameplayEventData EventData)
{
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, true);
}

void UGA_MeleeAttack::OnEventReceived(FGameplayTag EventTag, FGameplayEventData EventData)
{
	UE_LOG(LogTemp, Warning, TEXT("[GA_MeleeAttack] OnEventReceived: %s on %s"), *EventTag.ToString(), *GetName());

	// 把当前 GA 自身塞入 OptionalObject，供 TargetType 精确拿到 ActionData
	EventData.OptionalObject = this;

	// 拆分为两步以复用 ContainerSpec（目标数据 + 附加 Effect 需要同一批目标）
	FYogGameplayEffectContainerSpec ContainerSpec = MakeEffectContainerSpec(EventTag, EventData, -1);
	TArray<FActiveGameplayEffectHandle> Handles = ApplyEffectContainerSpec(ContainerSpec);

	AYogCharacterBase* Owner = Cast<AYogCharacterBase>(GetOwningActorFromActorInfo());

	// 若有 GE 命中目标，标记本节已命中（供 AN_EnemyComboSection::bRequireHit 使用）
	if (Handles.Num() > 0 && Owner)
	{
		Owner->bComboHitConnected = true;
	}

	// 应用来自 AN_MeleeDamage::AdditionalTargetEffects 的附加 Effect（复用同批目标）
	if (Owner && Owner->PendingAdditionalHitEffects.Num() > 0 && Handles.Num() > 0)
	{
		UAbilitySystemComponent* SelfASC = GetAbilitySystemComponentFromActorInfo();
		if (SelfASC)
		{
			for (TSubclassOf<UGameplayEffect> EffClass : Owner->PendingAdditionalHitEffects)
			{
				if (!EffClass) continue;
				FGameplayEffectSpecHandle Spec = MakeOutgoingGameplayEffectSpec(EffClass, GetAbilityLevel());
				if (Spec.IsValid())
				{
					// 应用到与主伤害 GE 相同的目标集合
					K2_ApplyGameplayEffectSpecToTarget(Spec, ContainerSpec.TargetData);
				}
			}
		}
	}

	// 消费附加 Effect 暂存（无论是否命中都清空，防止残留到下一次 Notify）
	if (Owner)
	{
		Owner->PendingAdditionalHitEffects.Empty();
	}
}
