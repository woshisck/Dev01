#include "AbilitySystem/Abilities/GA_PlayerSpecialAttack.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "AbilitySystem/Abilities/GA_Knockback.h"
#include "AbilitySystem/Abilities/YogAbilityTypes.h"
#include "AbilitySystem/AbilityTask/YogAbilityTask_PlayMontageAndWaitForEvent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Animation/AN_MeleeDamage.h"
#include "Animation/AnimInstance.h"
#include "Animation/HitStopManager.h"
#include "BuffFlow/BuffFlowComponent.h"
#include "Character/YogCharacterBase.h"
#include "Character/PlayerCharacterBase.h"
#include "Component/BufferComponent.h"
#include "Component/CombatItemComponent.h"
#include "Component/PlayerSpecialAttackComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Data/MontageConfigDA.h"
#include "Data/RuneDataAsset.h"

namespace
{
FGameplayTag GetDefaultSpecialAttackEventTag()
{
	return FGameplayTag::RequestGameplayTag(TEXT("GameplayEffect.DamageType.GeneralAttack"));
}

void GA_PlayerSpecialAttack_CollectHitActors(const FYogGameplayEffectContainerSpec& ContainerSpec, TArray<AActor*>& OutHitActors)
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
}

UGA_PlayerSpecialAttack::UGA_PlayerSpecialAttack(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	bRetriggerInstancedAbility = false;

	AbilityTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("Ability.Player.Special")));
	AbilityTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.Special")));
	AbilityTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("Ability.Player.SpecialAttack")));
	AbilityTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.SpecialAttack")));
	ActivationOwnedTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.Special")));
	ActivationBlockedTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("Buff.Status.Dead")));
	ActivationBlockedTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("Buff.Status.HitReact")));
	ActivationBlockedTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("Buff.Status.Knockback")));
	ActivationBlockedTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.Special")));
	ActivationBlockedTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.SpecialAttack")));

	EventTags.AddTag(GetDefaultSpecialAttackEventTag());
}

void UGA_PlayerSpecialAttack::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	PlayerOwner = ActorInfo ? Cast<APlayerCharacterBase>(ActorInfo->AvatarActor.Get()) : nullptr;
	// TODO: SpecialAttackComponent was unhooked from PlayerCharacter; rewire the special-attack
	// config source when this GA becomes the standalone Special() ability. Until then this GA
	// has no montage source and will early-out below.
	SpecialAttackComponent = nullptr;
	if (!PlayerOwner)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	ActiveCombatDeckContext = FCombatDeckActionContext();
	bHasActiveCombatDeckContext = false;
	bCombatDeckCardResolvedThisActivation = false;
	ActiveCombatCardResult = FCombatCardResolveResult();

	ActiveConfig = SpecialAttackComponent ? SpecialAttackComponent->GetSpecialAttackConfig() : FSpecialAttackConfig();
	ActiveMontage = ActiveConfig.Montage;
	if (!ActiveMontage)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GA_PlayerSpecialAttack] Missing montage on special attack owner=%s."), *GetNameSafe(PlayerOwner));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	CaptureCombatDeckContext();

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	AbilityActivationTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		const FGameplayTag SpecialTag = FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.Special"));
		if (SpecialTag.IsValid() && ASC->GetTagCount(SpecialTag) <= 0)
		{
			ASC->AddLooseGameplayTag(SpecialTag);
			bAddedSpecialAttackLooseTag = true;
		}

		const FGameplayTag CanComboTag = FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.CanCombo"));
		ASC->SetLooseGameplayTagCount(CanComboTag, 0);

		if (CanComboTagHandle.IsValid())
		{
			ASC->UnregisterGameplayTagEvent(CanComboTagHandle, CanComboTag, EGameplayTagEventType::NewOrRemoved);
		}
		CanComboTagHandle = ASC->RegisterGameplayTagEvent(
			CanComboTag,
			EGameplayTagEventType::NewOrRemoved
		).AddUObject(this, &UGA_PlayerSpecialAttack::OnCanComboTagChanged);
	}

	const float PlayRate = ActiveConfig.PlayRate > 0.0f ? ActiveConfig.PlayRate : 1.0f;
	FGameplayTagContainer MontageEventTags = EventTags;
	if (MontageEventTags.IsEmpty())
	{
		MontageEventTags.AddTag(GetDefaultSpecialAttackEventTag());
	}

	ActiveMontageTask = UYogAbilityTask_PlayMontageAndWaitForEvent::PlayMontageAndWaitForEvent(
		this,
		NAME_None,
		ActiveMontage,
		MontageEventTags,
		PlayRate,
		NAME_None,
		true,
		1.0f);

	if (!ActiveMontageTask)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	ActiveMontageTask->OnCompleted.AddDynamic(this, &UGA_PlayerSpecialAttack::OnMontageCompleted);
	ActiveMontageTask->OnBlendOut.AddDynamic(this, &UGA_PlayerSpecialAttack::OnMontageBlendOut);
	ActiveMontageTask->OnInterrupted.AddDynamic(this, &UGA_PlayerSpecialAttack::OnMontageInterrupted);
	ActiveMontageTask->OnCancelled.AddDynamic(this, &UGA_PlayerSpecialAttack::OnMontageCancelled);
	ActiveMontageTask->EventReceived.AddDynamic(this, &UGA_PlayerSpecialAttack::OnEventReceived);
	ActiveMontageTask->ReadyForActivation();
}

void UGA_PlayerSpecialAttack::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	ActiveMontageTask = nullptr;

	if (ActiveCombatCardResult.bHadCard && PlayerOwner && PlayerOwner->CombatDeckComponent)
	{
		PlayerOwner->CombatDeckComponent->StopCardFlow(ActiveCombatCardResult.ConsumedCard);
		PlayerOwner->CombatDeckComponent->StopCardFlow(ActiveCombatCardResult.LinkedSourceCard);
		PlayerOwner->CombatDeckComponent->StopCardFlow(ActiveCombatCardResult.LinkedTargetCard);
	}

	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		const FGameplayTag CanComboTag = FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.CanCombo"));
		if (CanComboTagHandle.IsValid())
		{
			ASC->UnregisterGameplayTagEvent(CanComboTagHandle, CanComboTag, EGameplayTagEventType::NewOrRemoved);
			CanComboTagHandle.Reset();
		}
		ASC->SetLooseGameplayTagCount(CanComboTag, 0);

		const FGameplayTag SpecialTag = FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.Special"), false);
		if (bAddedSpecialAttackLooseTag && SpecialTag.IsValid())
		{
			ASC->RemoveLooseGameplayTag(SpecialTag);
			bAddedSpecialAttackLooseTag = false;
		}
	}

	ActiveMontage = nullptr;
	PlayerOwner = nullptr;
	SpecialAttackComponent = nullptr;
	ActiveConfig = FSpecialAttackConfig();
	AbilityActivationTime = 0.0f;
	bAddedSpecialAttackLooseTag = false;
	bIsHandlingSpecialAttackEvent = false;
	bHasActiveCombatDeckContext = false;
	bCombatDeckCardResolvedThisActivation = false;
	ActiveCombatCardResult = FCombatCardResolveResult();
	ActiveCombatDeckContext = FCombatDeckActionContext();

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_PlayerSpecialAttack::OnCanComboTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	if (NewCount <= 0 || !PlayerOwner)
	{
		return;
	}

	UBufferComponent* Buffer = PlayerOwner->GetInputBufferComponent();
	if (!Buffer)
	{
		return;
	}

	if (!Buffer->ConsumeBufferedInputSince(EInputCommandType::Attack, AbilityActivationTime))
	{
		return;
	}

	Buffer->ClearBuffer();
	bool bActivated = false;
	if (UYogAbilitySystemComponent* PlayerASC = Cast<UYogAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo()))
	{
		bActivated = PlayerASC->TryActivateNextAttackComboAbility(true, true);
	}
	if (bActivated)
	{
		EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
	}
	else if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->SetLooseGameplayTagCount(Tag, 0);
	}
}

void UGA_PlayerSpecialAttack::OnMontageCompleted(FGameplayTag EventTag, FGameplayEventData EventData)
{
	K2_OnSpecialAttackEvent(EventTag, EventData);
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
}

void UGA_PlayerSpecialAttack::OnMontageBlendOut(FGameplayTag EventTag, FGameplayEventData EventData)
{
	K2_OnSpecialAttackEvent(EventTag, EventData);
	if (bEndWhenMontageBlendsOut)
	{
		EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
	}
}

void UGA_PlayerSpecialAttack::OnMontageInterrupted(FGameplayTag EventTag, FGameplayEventData EventData)
{
	K2_OnSpecialAttackEvent(EventTag, EventData);
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, true);
}

void UGA_PlayerSpecialAttack::OnMontageCancelled(FGameplayTag EventTag, FGameplayEventData EventData)
{
	K2_OnSpecialAttackEvent(EventTag, EventData);
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, true);
}

void UGA_PlayerSpecialAttack::OnEventReceived(FGameplayTag EventTag, FGameplayEventData EventData)
{
	K2_OnSpecialAttackEvent(EventTag, EventData);
	ApplyDamageFromEvent(EventTag, EventData);
}

void UGA_PlayerSpecialAttack::CaptureCombatDeckContext()
{
	if (!PlayerOwner)
	{
		return;
	}

	ActiveCombatDeckContext = FCombatDeckActionContext();
	ActiveCombatDeckContext.ActionType = ECardRequiredAction::Heavy;
	ActiveCombatDeckContext.ActionSlot = ECombatDeckActionSlot::WeaponSkill;
	ActiveCombatDeckContext.FlowRole = ECombatDeckFlowRole::Finisher;
	ActiveCombatDeckContext.WeaponDef = PlayerOwner->EquippedWeaponDef;
	ActiveCombatDeckContext.bIsComboFinisher = true;

	ActiveCombatDeckContext.TriggerTiming = ECombatCardTriggerTiming::OnHit;
	ActiveCombatDeckContext.ReleaseMode = ActiveCombatDeckContext.bIsComboFinisher
		? ECombatCardReleaseMode::Finisher
		: ECombatCardReleaseMode::Normal;
	if (!ActiveCombatDeckContext.AttackInstanceGuid.IsValid())
	{
		ActiveCombatDeckContext.AttackInstanceGuid = FGuid::NewGuid();
	}
	if (!ActiveCombatDeckContext.AbilityTag.IsValid())
	{
		for (const FGameplayTag& Tag : AbilityTags)
		{
			ActiveCombatDeckContext.AbilityTag = Tag;
			break;
		}
	}

	bHasActiveCombatDeckContext = PlayerOwner->CombatDeckComponent != nullptr;
}

void UGA_PlayerSpecialAttack::PrimeCombatDeckHitContext(const TArray<AActor*>& HitActors) const
{
	AYogCharacterBase* Owner = Cast<AYogCharacterBase>(GetAvatarActorFromActorInfo());
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
	BuffFlowComponent->LastEventContext.AttackDirection = UGA_Knockback::ResolveAttackDirectionFromSource(Owner);
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

FCombatCardResolveResult UGA_PlayerSpecialAttack::ResolveCombatDeckOnHit()
{
	FCombatCardResolveResult EmptyResult;
	if (bCombatDeckCardResolvedThisActivation || !bHasActiveCombatDeckContext || !PlayerOwner || !PlayerOwner->CombatDeckComponent)
	{
		return EmptyResult;
	}

	FCombatDeckActionContext Context = ActiveCombatDeckContext;
	Context.TriggerTiming = ECombatCardTriggerTiming::OnHit;
	if (!Context.AttackInstanceGuid.IsValid())
	{
		Context.AttackInstanceGuid = FGuid::NewGuid();
		ActiveCombatDeckContext.AttackInstanceGuid = Context.AttackInstanceGuid;
	}

	const FCombatCardResolveResult Result = PlayerOwner->CombatDeckComponent->ResolveAttackCardWithContext(Context);
	if (Result.bHadCard)
	{
		bCombatDeckCardResolvedThisActivation = true;
		ActiveCombatCardResult = Result;
	}
	return Result;
}

void UGA_PlayerSpecialAttack::ApplyDamageFromEvent(FGameplayTag EventTag, const FGameplayEventData& EventData)
{
	if (!bApplyDamageFromMontageEvents || bIsHandlingSpecialAttackEvent)
	{
		return;
	}

	TGuardValue<bool> ReentrancyGuard(bIsHandlingSpecialAttackEvent, true);

	const FYogGameplayEffectContainerSpec ContainerSpec = MakeEffectContainerSpec(EventTag, EventData, -1);
	TArray<AActor*> HitActors;
	GA_PlayerSpecialAttack_CollectHitActors(ContainerSpec, HitActors);

	PrimeCombatDeckHitContext(HitActors);
	ResolveCombatDeckOnHit();

	const TArray<FActiveGameplayEffectHandle> Handles = ApplyEffectContainerSpec(ContainerSpec);

	AYogCharacterBase* Owner = Cast<AYogCharacterBase>(GetAvatarActorFromActorInfo());
	if (!Owner)
	{
		return;
	}

	if (Handles.Num() <= 0)
	{
		Owner->PendingAdditionalHitRunes.Empty();
		Owner->PendingOnHitEventTags.Empty();
		Owner->PendingHitStopOverride = AYogCharacterBase::FPendingHitStopOverride();
		Owner->PendingHitImpactCueTag = FGameplayTag();
		return;
	}

	Owner->bComboHitConnected = true;

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
					const float Frozen = (Override.Mode == EHitStopMode::Freeze) ? Override.FrozenDuration : 0.0f;
					const float Slow = (Override.Mode == EHitStopMode::Slow) ? Override.SlowDuration : 0.0f;
					HitStop->RequestMontageHitStop(AnimInst, Frozen, Slow, Override.SlowRate, Override.CatchUpRate);
				}
			}
			Override.bActive = false;
		}

		static const FGameplayTag HitTag = FGameplayTag::RequestGameplayTag(TEXT("Ability.Event.Attack.Hit"));
		for (AActor* HitActor : HitActors)
		{
			FGameplayEventData Payload;
			Payload.Instigator = Owner;
			Payload.Target = HitActor;
			UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Owner, HitTag, Payload);
		}

		if (APlayerCharacterBase* PlayerCharacter = Cast<APlayerCharacterBase>(Owner))
		{
			if (PlayerCharacter->CombatItemComponent)
			{
				for (AActor* HitActor : HitActors)
				{
					PlayerCharacter->CombatItemComponent->ApplyOilBladeHitToTarget(HitActor);
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
				if (AYogCharacterBase* HitCharacter = Cast<AYogCharacterBase>(HitActor))
				{
					HitCharacter->ReceiveOnHitRune(RuneDA, Owner);
				}
			}
		}

		for (const FGameplayTag& PendingEventTag : Owner->PendingOnHitEventTags)
		{
			for (AActor* HitActor : HitActors)
			{
				FGameplayEventData Payload;
				Payload.Instigator = Owner;
				Payload.Target = HitActor;
				UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Owner, PendingEventTag, Payload);
			}
		}
	}

	if (Owner->PendingHitImpactCueTag.IsValid())
	{
		if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
		{
			FGameplayCueParameters CueParams;
			CueParams.Instigator = Owner;
			CueParams.EffectCauser = Owner;
			if (!HitActors.IsEmpty() && HitActors[0])
			{
				CueParams.Location = HitActors[0]->GetActorLocation();
				CueParams.Normal = (HitActors[0]->GetActorLocation() - Owner->GetActorLocation()).GetSafeNormal();
			}
			else
			{
				CueParams.Location = Owner->GetActorLocation();
			}
			ASC->ExecuteGameplayCue(Owner->PendingHitImpactCueTag, CueParams);
		}
		Owner->PendingHitImpactCueTag = FGameplayTag();
	}

	Owner->PendingAdditionalHitRunes.Empty();
	Owner->PendingOnHitEventTags.Empty();
	Owner->PendingHitStopOverride = AYogCharacterBase::FPendingHitStopOverride();
}
