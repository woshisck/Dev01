// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/GA_PlayMontage.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "AbilitySystem/AbilityTask/YogTask_PlayMontageAbility.h"
#include "AbilitySystem/Abilities/GA_Knockback.h"
#include "AbilitySystem/Abilities/YogAbilityTypes.h"
#include "Animation/AN_MeleeDamage.h"
#include "Animation/AnimInstance.h"
#include "Animation/HitStopManager.h"
#include "BuffFlow/BuffFlowComponent.h"
#include "Character/PlayerCharacterBase.h"
#include "Character/YogCharacterBase.h"
#include "Component/BufferComponent.h"
#include "Component/CombatItemComponent.h"
#include "Component/PlayerActiveSkillComponent.h"
#include "Component/CharacterDataComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Data/AbilityData.h"
#include "Data/CharacterData.h"
#include "Data/MontageConfigDA.h"
#include "Engine/World.h"

namespace
{
	void GA_PlayMontage_CollectHitActors(const FYogGameplayEffectContainerSpec& ContainerSpec, TArray<AActor*>& OutHitActors)
	{
		for (const TSharedPtr<FGameplayAbilityTargetData>& Data : ContainerSpec.TargetData.Data)
		{
			if (!Data.IsValid())
			{
				continue;
			}

			for (TWeakObjectPtr<AActor> WeakActor : Data->GetActors())
			{
				if (AActor* Actor = WeakActor.Get())
				{
					OutHitActors.AddUnique(Actor);
				}
			}
		}
	}

	UAN_MeleeDamage* FindFirstDamageNotify(UAnimMontage* Montage)
	{
		if (!Montage)
		{
			return nullptr;
		}

		for (FAnimNotifyEvent& NotifyEvent : Montage->Notifies)
		{
			if (UAN_MeleeDamage* Notify = Cast<UAN_MeleeDamage>(NotifyEvent.Notify))
			{
				return Notify;
			}
		}
		return nullptr;
	}

	bool TryActivatePlayMontageAbilityByTagName(UAbilitySystemComponent* ASC, const TCHAR* TagName)
	{
		if (!ASC || !TagName)
		{
			return false;
		}

		const FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName(TagName), false);
		if (!Tag.IsValid())
		{
			return false;
		}

		FGameplayTagContainer TagContainer;
		TagContainer.AddTag(Tag);
		return ASC->TryActivateAbilitiesByTag(TagContainer, true);
	}

}

UGA_PlayMontage::UGA_PlayMontage(const FObjectInitializer& ObjectInitializer)
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	bRetriggerInstancedAbility = true;
	HoldReleaseEventTag = FGameplayTag::RequestGameplayTag(TEXT("GameplayEvent.WeaponSkill.Release"), false);
}

void UGA_PlayMontage::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	ActiveMontage = nullptr;
	bActiveHoldMontage = false;
	bHoldReleaseReceived = false;
	bCombatDeckCardResolvedThisActivation = false;
	ActiveCombatCardResult = FCombatCardResolveResult();
	ActiveCombatDeckGuid = FGuid::NewGuid();

	if (ActivePlayMontageTask)
	{
		ActivePlayMontageTask->EndTask();
		ActivePlayMontageTask = nullptr;
	}

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		return;
	}

	UYogAbilitySystemComponent* ASC = Cast<UYogAbilitySystemComponent>(ActorInfo->AbilitySystemComponent);
	AYogCharacterBase* Owner = Cast<AYogCharacterBase>(ActorInfo->AvatarActor.Get());
	if (UBufferComponent* Buffer = Owner ? Owner->GetInputBufferComponent() : nullptr)
	{
		Buffer->ClearBuffer();
	}

	UAnimMontage* MontageToPlay = nullptr;
	FGameplayTag AbilityDataTag;
	{
		static const FGameplayTag PreferredAbilityTags[] = {
			FGameplayTag::RequestGameplayTag(TEXT("Character.State.Skill.WeaponSkill.Combo1"), false),
			FGameplayTag::RequestGameplayTag(TEXT("Character.State.Skill.WeaponSkill.Combo2"), false),
			FGameplayTag::RequestGameplayTag(TEXT("Character.State.Skill.WeaponSkill.Combo3"), false),
			FGameplayTag::RequestGameplayTag(TEXT("Character.State.Skill.WeaponSkill.Combo4"), false),
			FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.WeaponSkill.Combo1"), false),
			FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.WeaponSkill.Combo2"), false),
			FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.WeaponSkill.Combo3"), false),
			FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.WeaponSkill.Combo4"), false),
		};
		for (const FGameplayTag& PreferredTag : PreferredAbilityTags)
		{
			if (PreferredTag.IsValid() && AbilityTags.HasTagExact(PreferredTag))
			{
				AbilityDataTag = PreferredTag;
				break;
			}
		}
		if (!AbilityDataTag.IsValid())
		{
			for (const FGameplayTag& Tag : AbilityTags)
			{
				AbilityDataTag = Tag;
				break;
			}
		}

		UCharacterDataComponent* CDC = Owner ? Owner->GetCharacterDataComponent() : nullptr;
		UCharacterData* CharacterData = CDC ? CDC->GetCharacterData() : nullptr;
		MontageToPlay = (CharacterData && CharacterData->AbilityData && AbilityDataTag.IsValid())
			? CharacterData->AbilityData->GetMontage(AbilityDataTag)
			: nullptr;
	}

	if (!MontageToPlay)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GA_PlayMontage] No AbilityData montage on owner=%s Tag=%s."),
			*GetNameSafe(Owner),
			AbilityDataTag.IsValid() ? *AbilityDataTag.ToString() : TEXT("(none)"));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	ActiveMontage = MontageToPlay;
	StartSharedSkillCooldownIfConfigured();

	UAN_MeleeDamage* TemplateDamageNotify = FindFirstDamageNotify(MontageToPlay);

	AbilityActivationTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	ResolveCombatDeck(CombatDeckCommitTiming, bConsumeCombatDeckOnCommit);

	if (ASC && bListenForComboWindow)
	{
		const FGameplayTag CharacterCanComboTag = FGameplayTag::RequestGameplayTag(TEXT("Character.State.Window.CanCombo"));
		const FGameplayTag LegacyCanComboTag = FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.CanCombo"));
		ASC->SetLooseGameplayTagCount(CharacterCanComboTag, 0);
		ASC->SetLooseGameplayTagCount(LegacyCanComboTag, 0);

		if (CanComboTagHandle.IsValid())
		{
			ASC->UnregisterGameplayTagEvent(CanComboTagHandle, CharacterCanComboTag, EGameplayTagEventType::NewOrRemoved);
		}
		if (LegacyCanComboTagHandle.IsValid())
		{
			ASC->UnregisterGameplayTagEvent(LegacyCanComboTagHandle, LegacyCanComboTag, EGameplayTagEventType::NewOrRemoved);
			LegacyCanComboTagHandle.Reset();
		}
		CanComboTagHandle = ASC->RegisterGameplayTagEvent(
			CharacterCanComboTag,
			EGameplayTagEventType::NewOrRemoved
		).AddUObject(this, &UGA_PlayMontage::OnCanComboTagChanged);
		LegacyCanComboTagHandle = ASC->RegisterGameplayTagEvent(
			LegacyCanComboTag,
			EGameplayTagEventType::NewOrRemoved
		).AddUObject(this, &UGA_PlayMontage::OnCanComboTagChanged);
	}

	if (ASC && DynamicEffectClass)
	{
		if (UAN_MeleeDamage* DmgNotify = TemplateDamageNotify)
		{
			FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
			FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(DynamicEffectClass, GetAbilityLevel(), Context);
			if (SpecHandle.IsValid())
			{
				FGameplayEffectSpec* Spec = SpecHandle.Data.Get();
				Spec->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Attribute.ActDamage")), DmgNotify->ActDamage);
				Spec->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Attribute.ActRange")), DmgNotify->ActRange);
				Spec->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Attribute.ActResilience")), DmgNotify->ActResilience);
				Spec->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Attribute.ActDmgReduce")), DmgNotify->ActDmgReduce);
				ActiveEffectHandles.Add(ASC->ApplyGameplayEffectSpecToSelf(*Spec));
			}
		}
	}

	FGameplayTagContainer MontageEventTags;
	FName MontageStartSection = NAME_None;
	if (IsHoldMontageConfigured(MontageToPlay))
	{
		bActiveHoldMontage = true;
		MontageEventTags.AddTag(HoldReleaseEventTag);
		MontageEventTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("GameplayEvent.WeaponSkill.Blocked"), false));
		MontageEventTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("GameplayEvent.WeaponSkill.JustBlocked"), false));
		if (HasMontageSection(MontageToPlay, HoldStartSection))
		{
			MontageStartSection = HoldStartSection;
		}
	}

	UYogTask_PlayMontageAbility* PlayMontageTask = UYogTask_PlayMontageAbility::YogPlayMontageAbility(
		this,
		NAME_None,
		MontageToPlay,
		MontageEventTags,
		1.0f,
		MontageStartSection);
	if (!PlayMontageTask)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	PlayMontageTask->OnBlendOut.AddDynamic(this, &UGA_PlayMontage::OnMontageBlendOut);
	PlayMontageTask->OnCompleted.AddDynamic(this, &UGA_PlayMontage::OnMontageCompleted);
	PlayMontageTask->OnInterrupted.AddDynamic(this, &UGA_PlayMontage::OnMontageInterrupted);
	PlayMontageTask->OnCancelled.AddDynamic(this, &UGA_PlayMontage::OnMontageCancelled);
	PlayMontageTask->OnEventReceived.AddDynamic(this, &UGA_PlayMontage::OnEventReceived);

	ActivePlayMontageTask = PlayMontageTask;
	PlayMontageTask->ReadyForActivation();
	ConfigureHoldMontageSections();
	ScheduleBlockStartWindow();

}

void UGA_PlayMontage::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	ActivePlayMontageTask = nullptr;
	StopActiveCombatDeckFlows();
	ClearBlockStateTags();

	ActiveMontage = nullptr;
	bActiveHoldMontage = false;
	bHoldReleaseReceived = false;
	bCombatDeckCardResolvedThisActivation = false;
	ActiveCombatCardResult = FCombatCardResolveResult();
	ActiveCombatDeckGuid = FGuid();

	if (ActorInfo && ActorInfo->AbilitySystemComponent.IsValid())
	{
		for (const FActiveGameplayEffectHandle& EffectHandle : ActiveEffectHandles)
		{
			if (EffectHandle.IsValid())
			{
				ActorInfo->AbilitySystemComponent->RemoveActiveGameplayEffect(EffectHandle);
			}
		}
	}
	ActiveEffectHandles.Reset();

	if (UAbilitySystemComponent* ASCLocal = GetAbilitySystemComponentFromActorInfo())
	{
		const FGameplayTag CharacterCanComboTag = FGameplayTag::RequestGameplayTag(TEXT("Character.State.Window.CanCombo"));
		const FGameplayTag LegacyCanComboTag = FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.CanCombo"));
		if (CanComboTagHandle.IsValid())
		{
			ASCLocal->UnregisterGameplayTagEvent(CanComboTagHandle, CharacterCanComboTag, EGameplayTagEventType::NewOrRemoved);
			CanComboTagHandle.Reset();
		}
		if (LegacyCanComboTagHandle.IsValid())
		{
			ASCLocal->UnregisterGameplayTagEvent(LegacyCanComboTagHandle, LegacyCanComboTag, EGameplayTagEventType::NewOrRemoved);
			LegacyCanComboTagHandle.Reset();
		}
		if (bListenForComboWindow)
		{
			ASCLocal->SetLooseGameplayTagCount(CharacterCanComboTag, 0);
			ASCLocal->SetLooseGameplayTagCount(LegacyCanComboTag, 0);
		}
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_PlayMontage::OnMontageCompleted()
{
	HandleMontageEnded(false);
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
}

void UGA_PlayMontage::OnMontageBlendOut()
{
	HandleMontageEnded(false);
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
}

void UGA_PlayMontage::OnMontageInterrupted()
{
	HandleMontageEnded(true);
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
}

void UGA_PlayMontage::OnMontageCancelled()
{
	HandleMontageEnded(true);
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, true);
}

void UGA_PlayMontage::HandleMontageEnded(bool bWasCancelled)
{
}

void UGA_PlayMontage::OnEventReceived(FGameplayTag EventTag, const FGameplayEventData& EventData)
{
	if (bActiveHoldMontage && HoldReleaseEventTag.IsValid() && EventTag.MatchesTagExact(HoldReleaseEventTag))
	{
		HandleHoldInputReleased();
		return;
	}
	if (bActiveHoldMontage && EventTag.MatchesTagExact(FGameplayTag::RequestGameplayTag(TEXT("GameplayEvent.WeaponSkill.JustBlocked"), false)))
	{
		ApplyJustBlockReward();
		return;
	}
	if (bActiveHoldMontage && EventTag.MatchesTagExact(FGameplayTag::RequestGameplayTag(TEXT("GameplayEvent.WeaponSkill.Blocked"), false)))
	{
		return;
	}

	// Re-entry guard: downstream broadcasts (Ability.Event.Attack.Hit / CritHit, DamageExecution events)
	// can be caught by this same task if the BP's EventTags filter is too broad, causing infinite recursion.
	if (bIsHandlingMeleeEvent) return;
	TGuardValue<bool> ReentrancyGuard(bIsHandlingMeleeEvent, true);

	AYogCharacterBase* Owner = Cast<AYogCharacterBase>(GetOwningActorFromActorInfo());
	const UAN_MeleeDamage* FiredDamageNotify = Cast<UAN_MeleeDamage>(EventData.OptionalObject);
	FYogGameplayEffectContainerSpec ContainerSpec = MakeEffectContainerSpec(EventTag, EventData, -1);
	TArray<AActor*> HitActors;
	GA_PlayMontage_CollectHitActors(ContainerSpec, HitActors);
	if (Owner && HitActors.Num() > 0)
	{
		PrimeCombatDeckHitContext(Owner, HitActors);
		ResolveCombatDeck(ECombatCardTriggerTiming::OnHit, false);
	}

	TArray<FActiveGameplayEffectHandle> Handles = ApplyEffectContainerSpec(ContainerSpec);
	if (Handles.Num() > 0 && Owner)
	{
		Owner->bComboHitConnected = true;

		// AN_MeleeDamage 配置的 HitStop：命中至少一个目标时直接对攻击者蒙太奇生效
		if (HitActors.Num() > 0)
		{
			AYogCharacterBase::FPendingHitStopOverride& Override = Owner->PendingHitStopOverride;
			if (Override.bActive && Override.Mode != EHitStopMode::None)
			{
				UAnimInstance* AnimInst = Owner->GetMesh() ? Owner->GetMesh()->GetAnimInstance() : nullptr;
				if (AnimInst)
				{
					if (UHitStopManager* HitStop = Owner->GetWorld() ? Owner->GetWorld()->GetSubsystem<UHitStopManager>() : nullptr)
					{
						const float Frozen = (Override.Mode == EHitStopMode::Freeze) ? Override.FrozenDuration : 0.f;
						const float Slow   = (Override.Mode == EHitStopMode::Slow)   ? Override.SlowDuration   : 0.f;
						HitStop->RequestMontageHitStop(AnimInst, Frozen, Slow, Override.SlowRate, Override.CatchUpRate);
					}
				}
				Override.bActive = false;
			}
		}

		static const FGameplayTag HitTag = FGameplayTag::RequestGameplayTag(TEXT("Ability.Event.Attack.Hit"));
		for (AActor* HitActor : HitActors)
		{
			FGameplayEventData Payload;
			Payload.Instigator = Owner;
			Payload.Target = HitActor;
			UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Owner, HitTag, Payload);
		}

		// Temporarily disabled: hit freeze/slow now handles melee impact feedback.
		// if (FiredDamageNotify)
		// {
		// 	FiredDamageNotify->ApplyHitSuccessDilation(Owner);
		// }

		if (APlayerCharacterBase* PlayerOwner = Cast<APlayerCharacterBase>(Owner))
		{
			if (PlayerOwner->CombatItemComponent)
			{
				for (AActor* HitActor : HitActors)
				{
					PlayerOwner->CombatItemComponent->ApplyOilBladeHitToTarget(HitActor);
				}
			}
		}

		for (URuneDataAsset* RuneDA : Owner->PendingAdditionalHitRunes)
		{
			if (!RuneDA)
			{
				continue;
			}

			for (AActor* HitActor : HitActors)
			{
				if (AYogCharacterBase* HitChar = Cast<AYogCharacterBase>(HitActor))
				{
					HitChar->ReceiveOnHitRune(RuneDA, Owner);
				}
			}
		}

		for (const FGameplayTag& EvtTag : Owner->PendingOnHitEventTags)
		{
			for (AActor* HitActor : HitActors)
			{
				FGameplayEventData EvtPayload;
				EvtPayload.Instigator = Owner;
				EvtPayload.Target = HitActor;
				UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Owner, EvtTag, EvtPayload);
			}
		}
	}

	if (Owner)
	{
		Owner->PendingAdditionalHitRunes.Empty();
		Owner->PendingOnHitEventTags.Empty();
		Owner->PendingHitStopOverride = AYogCharacterBase::FPendingHitStopOverride();
	}
}

FGameplayTag UGA_PlayMontage::GetPrimaryAbilityTag() const
{
	for (const FGameplayTag& Tag : AbilityTags)
	{
		return Tag;
	}
	return FGameplayTag();
}

FCombatDeckActionContext UGA_PlayMontage::BuildCombatDeckContext(ECombatCardTriggerTiming TriggerTiming, bool bResolveOnCommit) const
{
	FCombatDeckActionContext Context;
	Context.ActionType = ECardRequiredAction::Any;
	Context.ActionSlot = CombatDeckActionSlot;
	Context.FlowRole = CombatDeckFlowRole;
	Context.ComboIndex = 0;
	Context.AbilityTag = GetPrimaryAbilityTag();
	Context.bIsComboFinisher = CombatDeckFlowRole == ECombatDeckFlowRole::Finisher;
	Context.ReleaseMode = Context.bIsComboFinisher ? ECombatCardReleaseMode::Finisher : ECombatCardReleaseMode::Normal;
	Context.TriggerTiming = TriggerTiming;
	Context.bConsumeOnCommit = TriggerTiming == ECombatCardTriggerTiming::OnCommit && bResolveOnCommit;
	Context.AttackInstanceGuid = ActiveCombatDeckGuid.IsValid() ? ActiveCombatDeckGuid : FGuid::NewGuid();

	if (const APlayerCharacterBase* Player = Cast<APlayerCharacterBase>(GetAvatarActorFromActorInfo()))
	{
		Context.WeaponDef = Player->EquippedWeaponDef;
	}

	return Context;
}

FCombatCardResolveResult UGA_PlayMontage::ResolveCombatDeck(ECombatCardTriggerTiming TriggerTiming, bool bResolveOnCommit)
{
	FCombatCardResolveResult EmptyResult;
	if (!bResolveCombatDeck)
	{
		return EmptyResult;
	}

	if (TriggerTiming == ECombatCardTriggerTiming::OnHit && bCombatDeckCardResolvedThisActivation)
	{
		return EmptyResult;
	}

	APlayerCharacterBase* Player = Cast<APlayerCharacterBase>(GetAvatarActorFromActorInfo());
	if (!Player)
	{
		Player = Cast<APlayerCharacterBase>(GetOwningActorFromActorInfo());
	}

	if (!Player || !Player->CombatDeckComponent)
	{
		return EmptyResult;
	}

	const FCombatDeckActionContext Context = BuildCombatDeckContext(TriggerTiming, bResolveOnCommit);
	const FCombatCardResolveResult Result = Player->CombatDeckComponent->ResolveAttackCardWithContext(Context);
	if (Result.bHadCard)
	{
		ActiveCombatCardResult = Result;
		bCombatDeckCardResolvedThisActivation = true;
	}
	return Result;
}

void UGA_PlayMontage::PrimeCombatDeckHitContext(AYogCharacterBase* Owner, const TArray<AActor*>& HitActors) const
{
	if (!Owner)
	{
		return;
	}

	UBuffFlowComponent* BuffFlowComponent = Owner->FindComponentByClass<UBuffFlowComponent>();
	if (!BuffFlowComponent)
	{
		return;
	}

	BuffFlowComponent->LastEventContext.DamageCauser = Owner;
	BuffFlowComponent->LastEventContext.DamageAmount = 0.f;
	BuffFlowComponent->LastEventContext.AttackDirection =
		UGA_Knockback::ResolveAttackDirectionFromSource(Owner);
	BuffFlowComponent->LastEventContext.DamageReceivers.Reset();

	AActor* FirstTarget = nullptr;
	for (AActor* HitActor : HitActors)
	{
		if (!IsValid(HitActor))
		{
			continue;
		}

		BuffFlowComponent->LastEventContext.DamageReceivers.AddUnique(HitActor);
		if (!FirstTarget)
		{
			FirstTarget = HitActor;
		}
	}

	BuffFlowComponent->LastEventContext.DamageReceiver = FirstTarget;
}

void UGA_PlayMontage::StopActiveCombatDeckFlows()
{
	if (!ActiveCombatCardResult.bHadCard)
	{
		return;
	}

	APlayerCharacterBase* Player = Cast<APlayerCharacterBase>(GetAvatarActorFromActorInfo());
	if (!Player)
	{
		Player = Cast<APlayerCharacterBase>(GetOwningActorFromActorInfo());
	}

	if (!Player || !Player->CombatDeckComponent)
	{
		return;
	}

	Player->CombatDeckComponent->StopCardFlow(ActiveCombatCardResult.ResolvedCard);
	Player->CombatDeckComponent->StopCardFlow(ActiveCombatCardResult.LinkedSourceCard);
	Player->CombatDeckComponent->StopCardFlow(ActiveCombatCardResult.LinkedTargetCard);
}

void UGA_PlayMontage::StartSharedSkillCooldownIfConfigured() const
{
	if (!bStartsSharedSkillCooldown)
	{
		return;
	}

	APlayerCharacterBase* Player = Cast<APlayerCharacterBase>(GetAvatarActorFromActorInfo());
	if (!Player || !Player->ActiveSkillComponent)
	{
		return;
	}

	const float CooldownDuration = FMath::Max(GetRemainingCooldownTime(), SharedSkillCooldownFallbackDuration);
	Player->ActiveSkillComponent->StartSharedSkillCooldown(CooldownDuration);
}

bool UGA_PlayMontage::HasMontageSection(const UAnimMontage* Montage, FName SectionName) const
{
	return Montage && !SectionName.IsNone() && Montage->GetSectionIndex(SectionName) != INDEX_NONE;
}

bool UGA_PlayMontage::IsHoldMontageConfigured(const UAnimMontage* Montage) const
{
	return bHoldMontageUntilInputRelease
		&& HoldReleaseEventTag.IsValid()
		&& HasMontageSection(Montage, HoldLoopSection)
		&& HasMontageSection(Montage, HoldEndSection);
}

void UGA_PlayMontage::ConfigureHoldMontageSections()
{
	if (!bActiveHoldMontage || !ActiveMontage)
	{
		return;
	}

	const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();
	UAnimInstance* AnimInstance = ActorInfo ? ActorInfo->GetAnimInstance() : nullptr;
	if (!AnimInstance)
	{
		return;
	}

	if (HasMontageSection(ActiveMontage, HoldStartSection))
	{
		AnimInstance->Montage_SetNextSection(HoldStartSection, HoldLoopSection, ActiveMontage);
	}
	AnimInstance->Montage_SetNextSection(HoldLoopSection, HoldLoopSection, ActiveMontage);
}

void UGA_PlayMontage::HandleHoldInputReleased()
{
	if (!bActiveHoldMontage || bHoldReleaseReceived || !ActiveMontage)
	{
		return;
	}
	bHoldReleaseReceived = true;

	const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();
	UAnimInstance* AnimInstance = ActorInfo ? ActorInfo->GetAnimInstance() : nullptr;
	if (!AnimInstance)
	{
		return;
	}

	if (HasMontageSection(ActiveMontage, HoldStartSection))
	{
		AnimInstance->Montage_SetNextSection(HoldStartSection, HoldEndSection, ActiveMontage);
	}
	AnimInstance->Montage_SetNextSection(HoldLoopSection, HoldEndSection, ActiveMontage);

	if (bJumpToHoldEndSectionOnRelease)
	{
		AnimInstance->Montage_JumpToSection(HoldEndSection, ActiveMontage);
	}

	ClearBlockStateTags();
}

void UGA_PlayMontage::SetBlockStateTag(const FGameplayTag& Tag, int32 Count)
{
	if (!Tag.IsValid())
	{
		return;
	}

	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->SetLooseGameplayTagCount(Tag, Count);
	}
}

void UGA_PlayMontage::ClearBlockStateTags()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(BlockStartWindowHandle);
	}

	SetBlockStateTag(FGameplayTag::RequestGameplayTag(TEXT("Character.State.Block.Start"), false), 0);
	SetBlockStateTag(FGameplayTag::RequestGameplayTag(TEXT("Character.State.Block.Idle"), false), 0);
	SetBlockStateTag(FGameplayTag::RequestGameplayTag(TEXT("PlayerState.Block.Start"), false), 0);
	SetBlockStateTag(FGameplayTag::RequestGameplayTag(TEXT("PlayerState.Block.Idle"), false), 0);
}

void UGA_PlayMontage::ScheduleBlockStartWindow()
{
	if (!bActiveHoldMontage || !ActiveMontage)
	{
		return;
	}

	SetBlockStateTag(FGameplayTag::RequestGameplayTag(TEXT("Character.State.Block.Start"), false), 1);
	SetBlockStateTag(FGameplayTag::RequestGameplayTag(TEXT("Character.State.Block.Idle"), false), 0);
	SetBlockStateTag(FGameplayTag::RequestGameplayTag(TEXT("PlayerState.Block.Start"), false), 1);
	SetBlockStateTag(FGameplayTag::RequestGameplayTag(TEXT("PlayerState.Block.Idle"), false), 0);

	const int32 StartSectionIndex = HasMontageSection(ActiveMontage, HoldStartSection)
		? ActiveMontage->GetSectionIndex(HoldStartSection)
		: INDEX_NONE;
	const float StartWindowDuration = StartSectionIndex != INDEX_NONE
		? FMath::Max(ActiveMontage->GetSectionLength(StartSectionIndex), 0.01f)
		: 0.01f;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			BlockStartWindowHandle,
			this,
			&UGA_PlayMontage::FinishBlockStartWindow,
			StartWindowDuration,
			false);
	}
}

void UGA_PlayMontage::FinishBlockStartWindow()
{
	if (!bActiveHoldMontage || bHoldReleaseReceived)
	{
		ClearBlockStateTags();
		return;
	}

	SetBlockStateTag(FGameplayTag::RequestGameplayTag(TEXT("Character.State.Block.Start"), false), 0);
	SetBlockStateTag(FGameplayTag::RequestGameplayTag(TEXT("Character.State.Block.Idle"), false), 1);
	SetBlockStateTag(FGameplayTag::RequestGameplayTag(TEXT("PlayerState.Block.Start"), false), 0);
	SetBlockStateTag(FGameplayTag::RequestGameplayTag(TEXT("PlayerState.Block.Idle"), false), 1);
}

void UGA_PlayMontage::ApplyJustBlockReward()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC)
	{
		return;
	}

	AYogCharacterBase* Owner = Cast<AYogCharacterBase>(GetAvatarActorFromActorInfo());
	UCharacterDataComponent* CDC = Owner ? Owner->GetCharacterDataComponent() : nullptr;
	UCharacterData* CharacterData = CDC ? CDC->GetCharacterData() : nullptr;
	const UAbilityData* AbilityData = CharacterData ? CharacterData->AbilityData.Get() : nullptr;
	if (!AbilityData)
	{
		return;
	}

	const FGameplayTag BlockedTag = FGameplayTag::RequestGameplayTag(TEXT("Action.HitReact.Blocked"), false);
	const FGameplayTag JustBlockedTag = FGameplayTag::RequestGameplayTag(TEXT("GameplayEvent.WeaponSkill.JustBlocked"), false);
	const FPassiveActionData BlockedData = BlockedTag.IsValid() ? AbilityData->GetPassiveAbility(BlockedTag) : FPassiveActionData();
	bool bAppliedReward = false;
	for (const FYogApplyEffect& Effect : BlockedData.UniqueEffects)
	{
		if (!Effect.GameplayEffect)
		{
			continue;
		}
		if (Effect.TriggerTag.IsValid() && JustBlockedTag.IsValid() && !Effect.TriggerTag.MatchesTagExact(JustBlockedTag))
		{
			continue;
		}

		FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
		Context.AddSourceObject(this);
		FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(
			Effect.GameplayEffect,
			Effect.level > 0 ? Effect.level : GetAbilityLevel(),
			Context);
		if (SpecHandle.IsValid() && SpecHandle.Data.IsValid())
		{
			SpecHandle.Data->SetDuration(FMath::Max(JustBlockRewardDuration, 0.01f), true);
			ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
			bAppliedReward = true;
		}
	}

	if (!bAppliedReward)
	{
		UE_LOG(LogTemp, Verbose, TEXT("[Block] No JustBlock reward effect configured in AbilityData PassiveMap[%s]."), *BlockedTag.ToString());
	}
}

void UGA_PlayMontage::OnCanComboTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	if (NewCount <= 0)
	{
		return;
	}

	AYogCharacterBase* Owner = Cast<AYogCharacterBase>(GetAvatarActorFromActorInfo());
	if (!Owner)
	{
		return;
	}

	UBufferComponent* Buffer = Owner->GetInputBufferComponent();
	if (!Buffer)
	{
		return;
	}

	UYogAbilitySystemComponent* ASC = Cast<UYogAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo());
	const FGameplayTag CharacterCanComboTag = FGameplayTag::RequestGameplayTag(TEXT("Character.State.Window.CanCombo"));
	const FGameplayTag LegacyCanComboTag = FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.CanCombo"));

	EInputCommandType BufferedActionType = EInputCommandType::Attack;
	if (Buffer->ConsumeLatestActionInputSince(AbilityActivationTime, BufferedActionType))
	{
		Buffer->ClearBuffer();
		bool bActivated = false;
		const TCHAR* CharacterTagName = TEXT("Character.State.Skill.Attack");
		const TCHAR* LegacyTagName = TEXT("PlayerState.AbilityCast.Attack");
		bool bUseFallbackTag = true;
		switch (BufferedActionType)
		{
		case EInputCommandType::Attack:
			bUseFallbackTag = false;
			bActivated = ASC ? ASC->TryActivateNextAttackComboAbility(true, true) : false;
			break;
		case EInputCommandType::WeaponSkill:
			bUseFallbackTag = false;
			bActivated = ASC ? ASC->TryActivateNextWeaponSkillComboAbility(true, true) : false;
			break;
		case EInputCommandType::Dash:
			CharacterTagName = TEXT("Character.State.Movement.Dash");
			LegacyTagName = TEXT("PlayerState.AbilityCast.Dash");
			break;
		case EInputCommandType::Skill:
			bUseFallbackTag = false;
			if (APlayerCharacterBase* PlayerOwner = Cast<APlayerCharacterBase>(Owner))
			{
				bActivated = PlayerOwner->ActiveSkillComponent
					? PlayerOwner->ActiveSkillComponent->UseActiveSkill()
					: false;
			}
			break;
		default:
			break;
		}

		if (bUseFallbackTag)
		{
			bActivated = TryActivatePlayMontageAbilityByTagName(Owner->GetASC(), CharacterTagName)
				|| TryActivatePlayMontageAbilityByTagName(Owner->GetASC(), LegacyTagName);
		}
		if (!bActivated && ASC)
		{
			ASC->SetLooseGameplayTagCount(CharacterCanComboTag, 0);
			ASC->SetLooseGameplayTagCount(LegacyCanComboTag, 0);
		}
		return;
	}
}
