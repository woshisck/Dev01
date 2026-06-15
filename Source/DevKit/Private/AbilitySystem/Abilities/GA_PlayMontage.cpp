// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/GA_PlayMontage.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "AbilitySystem/AbilityTask/YogTask_PlayMontageAbility.h"
#include "AbilitySystem/Abilities/YogAbilityTypes.h"
#include "Animation/AN_MeleeDamage.h"
#include "Animation/AnimInstance.h"
#include "Animation/HitStopManager.h"
#include "Character/PlayerCharacterBase.h"
#include "Character/YogCharacterBase.h"
#include "Component/BufferComponent.h"
#include "Component/CombatItemComponent.h"
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

}

UGA_PlayMontage::UGA_PlayMontage(const FObjectInitializer& ObjectInitializer)
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	bRetriggerInstancedAbility = true;
}

void UGA_PlayMontage::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	ActiveMontage = nullptr;

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

	UAnimMontage* MontageToPlay = nullptr;
	FGameplayTag AbilityDataTag;
	{
		static const FGameplayTag PreferredAbilityTags[] = {
			FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.WeaponSkill.Combo1"), false),
			FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.WeaponSkill.Combo2"), false),
			FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.WeaponSkill.Combo3"), false),
			FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.WeaponSkill.Combo4"), false),
			FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.Special.Combo1"), false),
			FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.Special.Combo2"), false),
			FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.Special.Combo3"), false),
			FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.Special.Combo4"), false),
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

	UAN_MeleeDamage* TemplateDamageNotify = FindFirstDamageNotify(MontageToPlay);

	AbilityActivationTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;

	if (ASC)
	{
		const FGameplayTag CanComboTag = FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.CanCombo"));
		ASC->SetLooseGameplayTagCount(CanComboTag, 0);

		if (CanComboTagHandle.IsValid())
		{
			ASC->UnregisterGameplayTagEvent(CanComboTagHandle, CanComboTag, EGameplayTagEventType::NewOrRemoved);
		}
		CanComboTagHandle = ASC->RegisterGameplayTagEvent(
			CanComboTag,
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

	UYogTask_PlayMontageAbility* PlayMontageTask = UYogTask_PlayMontageAbility::YogPlayMontageAbility(
		this,
		NAME_None,
		MontageToPlay,
		FGameplayTagContainer(),
		1.0f,
		NAME_None);
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

}

void UGA_PlayMontage::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	ActivePlayMontageTask = nullptr;

	ActiveMontage = nullptr;

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
		const FGameplayTag CanComboTag = FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.CanCombo"));
		if (CanComboTagHandle.IsValid())
		{
			ASCLocal->UnregisterGameplayTagEvent(CanComboTagHandle, CanComboTag, EGameplayTagEventType::NewOrRemoved);
			CanComboTagHandle.Reset();
		}
		ASCLocal->SetLooseGameplayTagCount(CanComboTag, 0);
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_PlayMontage::OnMontageCompleted()
{
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
}

void UGA_PlayMontage::OnMontageBlendOut()
{
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
}

void UGA_PlayMontage::OnMontageInterrupted()
{
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
}

void UGA_PlayMontage::OnMontageCancelled()
{
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, true);
}

void UGA_PlayMontage::OnEventReceived(FGameplayTag EventTag, const FGameplayEventData& EventData)
{
	// Re-entry guard: downstream broadcasts (Ability.Event.Attack.Hit / CritHit, DamageExecution events)
	// can be caught by this same task if the BP's EventTags filter is too broad, causing infinite recursion.
	if (bIsHandlingMeleeEvent) return;
	TGuardValue<bool> ReentrancyGuard(bIsHandlingMeleeEvent, true);

	FYogGameplayEffectContainerSpec ContainerSpec = MakeEffectContainerSpec(EventTag, EventData, -1);
	TArray<FActiveGameplayEffectHandle> Handles = ApplyEffectContainerSpec(ContainerSpec);

	AYogCharacterBase* Owner = Cast<AYogCharacterBase>(GetOwningActorFromActorInfo());
	const UAN_MeleeDamage* FiredDamageNotify = Cast<UAN_MeleeDamage>(EventData.OptionalObject);
	if (Handles.Num() > 0 && Owner)
	{
		Owner->bComboHitConnected = true;

		TArray<AActor*> HitActors;
		GA_PlayMontage_CollectHitActors(ContainerSpec, HitActors);

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
	const FGameplayTag CanComboTag = FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.CanCombo"));

	EInputCommandType BufferedActionType = EInputCommandType::Attack;
	if (Buffer->ConsumeLatestActionInputSince(AbilityActivationTime, BufferedActionType))
	{
		Buffer->ClearBuffer();
		bool bActivated = false;
		if (ASC && (BufferedActionType == EInputCommandType::Attack || BufferedActionType == EInputCommandType::WeaponSkill))
		{
			const bool bHasActiveComboTag = BufferedActionType == EInputCommandType::Attack
				? ASC->HasActiveAttackComboAbilityTag()
				: ASC->HasActiveWeaponSkillComboAbilityTag();
			bActivated = BufferedActionType == EInputCommandType::Attack
				? ASC->TryActivateNextAttackComboAbility(true, true)
				: ASC->TryActivateNextWeaponSkillComboAbility(true, true);
			if (!bActivated && bHasActiveComboTag)
			{
				ASC->SetLooseGameplayTagCount(CanComboTag, 0);
				return;
			}
		}
		if (!bActivated)
		{
			const TCHAR* FallbackTagName = TEXT("PlayerState.AbilityCast.Attack");
			switch (BufferedActionType)
			{
			case EInputCommandType::WeaponSkill:
				FallbackTagName = TEXT("PlayerState.AbilityCast.WeaponSkill");
				break;
			case EInputCommandType::Dash:
				FallbackTagName = TEXT("PlayerState.AbilityCast.Dash");
				break;
			case EInputCommandType::Special:
				FallbackTagName = TEXT("PlayerState.AbilityCast.Special");
				break;
			default:
				break;
			}

			FGameplayTagContainer TagContainer;
			TagContainer.AddTag(FGameplayTag::RequestGameplayTag(FName(FallbackTagName)));
			bActivated = Owner->GetASC()->TryActivateAbilitiesByTag(TagContainer, true);
		}
		if (!bActivated && ASC)
		{
			ASC->SetLooseGameplayTagCount(CanComboTag, 0);
		}
		return;
	}
}
