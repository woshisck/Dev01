#include "AbilitySystem/Abilities/GA_MeleeAttack.h"
#include "AbilitySystem/AbilityTask/YogAbilityTask_PlayMontageAndWaitForEvent.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "AbilitySystem/Attribute/BaseAttributeSet.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Character/YogCharacterBase.h"
#include "Character/PlayerCharacterBase.h"
#include "Component/CombatDeckComponent.h"
#include "Component/CharacterDataComponent.h"
#include "Data/CharacterData.h"
#include "Data/AbilityData.h"
#include "Data/RuneDataAsset.h"

UGA_MeleeAttack::UGA_MeleeAttack()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	// 受击硬直 / 击退期间不允许发动攻击
	ActivationBlockedTags.AddTag(FGameplayTag::RequestGameplayTag("Buff.Status.HitReact"));
	ActivationBlockedTags.AddTag(FGameplayTag::RequestGameplayTag("Buff.Status.Knockback"));
}

UAN_MeleeDamage* UGA_MeleeAttack::GetFirstDamageNotify(UAnimMontage* Montage)
{
	if (!Montage) return nullptr;
	for (FAnimNotifyEvent& Event : Montage->Notifies)
	{
		if (UAN_MeleeDamage* DmgNotify = Cast<UAN_MeleeDamage>(Event.Notify))
			return DmgNotify;
	}
	return nullptr;
}

FActionData UGA_MeleeAttack::GetAbilityActionData_Implementation() const
{
	return CachedDamageNotify ? CachedDamageNotify->BuildActionData() : FActionData();
}

ECardRequiredAction UGA_MeleeAttack::GetCombatDeckActionType() const
{
	const FGameplayTag HeavyAttackTag = FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.HeavyAtk"));
	const FGameplayTag LightAttackTag = FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.LightAtk"));

	if (AbilityTags.HasTag(HeavyAttackTag))
	{
		return ECardRequiredAction::Heavy;
	}

	if (AbilityTags.HasTag(LightAttackTag))
	{
		return ECardRequiredAction::Light;
	}

	return ECardRequiredAction::Any;
}

bool UGA_MeleeAttack::IsCombatDeckComboFinisher() const
{
	const FGameplayTag LightFinisherTag = FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.LightAtk.Combo4"));
	const FGameplayTag HeavyFinisherTag = FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.HeavyAtk.Combo4"));

	return AbilityTags.HasTagExact(LightFinisherTag) || AbilityTags.HasTagExact(HeavyFinisherTag);
}

void UGA_MeleeAttack::SetNextActivationFromDashSave(bool bFromDashSave)
{
	bNextActivationFromDashSave = bFromDashSave;
}

void UGA_MeleeAttack::TryResolveCombatDeckOnHit()
{
	if (bCombatDeckCardResolvedThisActivation)
	{
		return;
	}

	APlayerCharacterBase* PlayerOwner = Cast<APlayerCharacterBase>(GetAvatarActorFromActorInfo());
	if (!PlayerOwner)
	{
		PlayerOwner = Cast<APlayerCharacterBase>(GetOwningActorFromActorInfo());
	}

	if (!PlayerOwner || !PlayerOwner->CombatDeckComponent)
	{
		return;
	}

	bCombatDeckCardResolvedThisActivation = true;
	PlayerOwner->CombatDeckComponent->ResolveAttackCard(
		GetCombatDeckActionType(),
		IsCombatDeckComboFinisher(),
		bCombatDeckFromDashSave);
}

void UGA_MeleeAttack::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	bCombatDeckCardResolvedThisActivation = false;
	bCombatDeckFromDashSave = bNextActivationFromDashSave;
	bNextActivationFromDashSave = false;

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	UE_LOG(LogTemp, Warning, TEXT("[GA_MeleeAttack] ActivateAbility: %s"), *GetName());

	// 玩家 GA：检查消耗/冷却
	if (bRequireCommit && !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 从 MontageMap 读取蒙太奇
	AYogCharacterBase* ActivateOwner = Cast<AYogCharacterBase>(GetOwningActorFromActorInfo());
	UCharacterDataComponent* CDC = ActivateOwner ? ActivateOwner->GetCharacterDataComponent() : nullptr;
	UCharacterData* CD = CDC ? CDC->GetCharacterData() : nullptr;

	FGameplayTag FirstTag;
	for (const FGameplayTag& Tag : AbilityTags) { FirstTag = Tag; break; }

	UAnimMontage* Montage = (CD && CD->AbilityData && FirstTag.IsValid())
		? CD->AbilityData->GetMontage(FirstTag) : nullptr;

	// 缓存第一个 AN_MeleeDamage，后续 GetAbilityActionData / StatAfterATK 使用
	CachedDamageNotify = GetFirstDamageNotify(Montage);

	// 施加攻击前摇 GE（玩家 GA 配置，敌人 GA 留空跳过）
	if (StatBeforeATKEffect && CachedDamageNotify)
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
				SpecHandle.Data->SetSetByCallerMagnitude(TAG_ActDamage,    CachedDamageNotify->ActDamage);
				SpecHandle.Data->SetSetByCallerMagnitude(TAG_ActRange,     CachedDamageNotify->ActRange);
				SpecHandle.Data->SetSetByCallerMagnitude(TAG_ActRes,       CachedDamageNotify->ActResilience);
				SpecHandle.Data->SetSetByCallerMagnitude(TAG_ActDmgReduce, CachedDamageNotify->ActDmgReduce);
				StatBeforeATKHandle = ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
			}
		}
	}

	// 重置命中标志（为本次攻击的第一节做准备）
	if (ActivateOwner)
	{
		ActivateOwner->bComboHitConnected = false;
	}

	if (!Montage)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GA_MeleeAttack] No Montage found for %s — ability ended immediately."), *GetName());
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	// 监听 AnimNotify 伤害事件：必须明确传入 Tag，空容器不会注册任何监听
	FGameplayTagContainer DamageEventTags;
	DamageEventTags.AddTag(FGameplayTag::RequestGameplayTag(FName("GameplayEffect.DamageType.GeneralAttack")));

	// 读取 AttackSpeed 属性作为蒙太奇播放速率
	float AttackSpeedRate = 1.0f;
	if (UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get())
	{
		bool bFound = false;
		const float SpeedValue = ASC->GetGameplayAttributeValue(UBaseAttributeSet::GetAttackSpeedAttribute(), bFound);
		if (bFound && SpeedValue > 0.f)
		{
			AttackSpeedRate = SpeedValue;
		}
	}

	// 创建复合任务：播放蒙太奇 + 监听 GameplayEvent（AnimNotify 触发伤害事件）
	UYogAbilityTask_PlayMontageAndWaitForEvent* Task =
		UYogAbilityTask_PlayMontageAndWaitForEvent::PlayMontageAndWaitForEvent(
			this,
			NAME_None,
			Montage,
			DamageEventTags,
			AttackSpeedRate,
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

	// 安全清理：技能结束时清空未消费的暂存数据（蒙太奇被打断未触发 OnEventReceived 时保护用）
	if (AYogCharacterBase* Owner = Cast<AYogCharacterBase>(GetOwningActorFromActorInfo()))
	{
		Owner->PendingAdditionalHitRunes.Empty();
		Owner->PendingOnHitEventTags.Empty();
	}

	// 移除攻击前摇 GE
	if (StatBeforeATKHandle.IsValid())
	{
		if (ASC) ASC->RemoveActiveGameplayEffect(StatBeforeATKHandle);
		StatBeforeATKHandle = FActiveGameplayEffectHandle();
	}

	// 施加攻击后摇 GE（仅正常结束时，Cancel/Interrupt 不触发）
	// 优先用最后命中的 Notify 数据（多段命中代表最后一击），未命中过则 fallback 到第一个 Notify。
	const UAN_MeleeDamage* AfterATKNotify = LastFiredDamageNotify ? LastFiredDamageNotify : CachedDamageNotify.Get();
	if (!bWasCancelled && StatAfterATKEffect && ASC && AfterATKNotify)
	{
		static const FGameplayTag TAG_ActDamage    = FGameplayTag::RequestGameplayTag("Attribute.ActDamage");
		static const FGameplayTag TAG_ActRange     = FGameplayTag::RequestGameplayTag("Attribute.ActRange");
		static const FGameplayTag TAG_ActRes       = FGameplayTag::RequestGameplayTag("Attribute.ActResilience");
		static const FGameplayTag TAG_ActDmgReduce = FGameplayTag::RequestGameplayTag("Attribute.ActDmgReduce");

		FGameplayEffectContextHandle Ctx = ASC->MakeEffectContext();
		Ctx.AddSourceObject(this);
		FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(StatAfterATKEffect, GetAbilityLevel(), Ctx);
		if (Spec.IsValid())
		{
			Spec.Data->SetSetByCallerMagnitude(TAG_ActDamage,    AfterATKNotify->ActDamage);
			Spec.Data->SetSetByCallerMagnitude(TAG_ActRange,     AfterATKNotify->ActRange);
			Spec.Data->SetSetByCallerMagnitude(TAG_ActRes,       AfterATKNotify->ActResilience);
			Spec.Data->SetSetByCallerMagnitude(TAG_ActDmgReduce, AfterATKNotify->ActDmgReduce);
			ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
		}
	}
	CachedDamageNotify    = nullptr;
	LastFiredDamageNotify = nullptr;

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

	// OptionalObject 已由 AN_MeleeDamage::Notify 设置为发射该事件的 Notify 本身，
	// YogTargetType_Melee::GetActionData 从中读取 HitboxTypes / ActRange 等参数。
	// 同时更新 LastFiredDamageNotify，供 StatAfterATK 使用（多段命中时代表最后一击）。
	if (const UAN_MeleeDamage* FiredNotify = Cast<const UAN_MeleeDamage>(EventData.OptionalObject))
	{
		LastFiredDamageNotify = FiredNotify;

		// 动作韧性：命中窗口期间攻击方韧性临时提升，ReceiveDamage 读取后自动清零
		if (UYogAbilitySystemComponent* SourceASC = Cast<UYogAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo()))
			SourceASC->CurrentActionPoiseBonus = FiredNotify->ActResilience;
	}

	// 拆分为两步以复用 ContainerSpec（目标数据 + 附加 Effect 需要同一批目标）
	FYogGameplayEffectContainerSpec ContainerSpec = MakeEffectContainerSpec(EventTag, EventData, -1);
	TArray<FActiveGameplayEffectHandle> Handles = ApplyEffectContainerSpec(ContainerSpec);

	AYogCharacterBase* Owner = Cast<AYogCharacterBase>(GetOwningActorFromActorInfo());

	// 若有 GE 命中目标，标记本节已命中（供 AN_EnemyComboSection::bRequireHit 使用）
	if (Handles.Num() > 0 && Owner)
	{
		Owner->bComboHitConnected = true;
		TryResolveCombatDeckOnHit();

		// 收集本次命中的所有目标（供 Ability.Event 广播和 AdditionalRuneEffects 共用）
		TArray<AActor*> HitActors;
		for (const TSharedPtr<FGameplayAbilityTargetData>& Data : ContainerSpec.TargetData.Data)
		{
			if (Data.IsValid())
			{
				for (TWeakObjectPtr<AActor> WeakActor : Data->GetActors())
				{
					if (AActor* Actor = WeakActor.Get())
						HitActors.AddUnique(Actor);
				}
			}
		}

		// 广播 Ability.Event.Attack.Hit 给攻击者（BGC 事件驱动型符文监听此事件）
		static const FGameplayTag HitTag = FGameplayTag::RequestGameplayTag(TEXT("Ability.Event.Attack.Hit"));
		for (AActor* HitActor : HitActors)
		{
			FGameplayEventData Payload;
			Payload.Instigator = Owner;
			Payload.Target     = HitActor;
			UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Owner, HitTag, Payload);
		}

		// 触发来自 AN_MeleeDamage::AdditionalRuneEffects 的附加符文（复用同批目标）
		for (URuneDataAsset* RuneDA : Owner->PendingAdditionalHitRunes)
		{
			if (!RuneDA) continue;
			for (AActor* HitActor : HitActors)
			{
				if (AYogCharacterBase* HitChar = Cast<AYogCharacterBase>(HitActor))
				{
					HitChar->ReceiveOnHitRune(RuneDA, Owner); // Owner = 攻击发起者
				}
			}
		}

		// 广播来自 AN_MeleeDamage::OnHitEventTags 的命中事件（供镜头抖动、音效等系统监听）
		for (const FGameplayTag& EvtTag : Owner->PendingOnHitEventTags)
		{
			for (AActor* HitActor : HitActors)
			{
				FGameplayEventData EvtPayload;
				EvtPayload.Instigator = Owner;
				EvtPayload.Target     = HitActor;
				UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Owner, EvtTag, EvtPayload);
			}
		}
	}

	// 消费暂存数据（无论是否命中都清空，防止残留到下一次 Notify）
	if (Owner)
	{
		Owner->PendingAdditionalHitRunes.Empty();
		Owner->PendingOnHitEventTags.Empty();
	}
}
