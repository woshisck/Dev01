#include "AbilitySystem/Abilities/GA_PlayerSpecialAttack.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/Abilities/YogAbilityTypes.h"
#include "AbilitySystem/AbilityTask/YogAbilityTask_PlayMontageAndWaitForEvent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Animation/AN_MeleeDamage.h"
#include "Animation/AnimInstance.h"
#include "Animation/HitStopManager.h"
#include "Character/YogCharacterBase.h"
#include "Character/PlayerCharacterBase.h"
#include "Component/BufferComponent.h"
#include "Component/ComboRuntimeComponent.h"
#include "Component/CombatItemComponent.h"
#include "Component/PlayerSpecialAttackComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Data/RuneDataAsset.h"

namespace
{
FGameplayTag GetDefaultSpecialAttackEventTag()
{
	return FGameplayTag::RequestGameplayTag(TEXT("GameplayEffect.DamageType.GeneralAttack"));
}

void CollectHitActors(const FYogGameplayEffectContainerSpec& ContainerSpec, TArray<AActor*>& OutHitActors)
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

	AbilityTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("Ability.Player.SpecialAttack")));
	AbilityTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.SpecialAttack")));
	ActivationOwnedTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.SpecialAttack")));
	ActivationBlockedTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("Buff.Status.Dead")));
	ActivationBlockedTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("Buff.Status.HitReact")));
	ActivationBlockedTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("Buff.Status.Knockback")));
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
	SpecialAttackComponent = PlayerOwner ? PlayerOwner->SpecialAttackComponent.Get() : nullptr;
	if (!PlayerOwner || !SpecialAttackComponent)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	ActiveConfig = SpecialAttackComponent->GetSpecialAttackConfig();
	ActiveMontage = ActiveConfig.Montage;
	if (!ActiveMontage)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GA_PlayerSpecialAttack] Missing montage on special attack owner=%s."), *GetNameSafe(PlayerOwner));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (PlayerOwner->ComboRuntimeComponent)
	{
		PlayerOwner->ComboRuntimeComponent->ResetCombo();
	}

	AbilityActivationTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		const FGameplayTag SpecialAttackTag = FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.SpecialAttack"));
		if (SpecialAttackTag.IsValid() && ASC->GetTagCount(SpecialAttackTag) <= 0)
		{
			ASC->AddLooseGameplayTag(SpecialAttackTag);
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

	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		const FGameplayTag CanComboTag = FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.CanCombo"));
		if (CanComboTagHandle.IsValid())
		{
			ASC->UnregisterGameplayTagEvent(CanComboTagHandle, CanComboTag, EGameplayTagEventType::NewOrRemoved);
			CanComboTagHandle.Reset();
		}
		ASC->SetLooseGameplayTagCount(CanComboTag, 0);

		const FGameplayTag SpecialAttackTag = FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.SpecialAttack"), false);
		if (bAddedSpecialAttackLooseTag && SpecialAttackTag.IsValid())
		{
			ASC->RemoveLooseGameplayTag(SpecialAttackTag);
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

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_PlayerSpecialAttack::OnCanComboTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	if (NewCount <= 0 || !PlayerOwner || !PlayerOwner->ComboRuntimeComponent)
	{
		return;
	}

	UBufferComponent* Buffer = PlayerOwner->GetInputBufferComponent();
	if (!Buffer)
	{
		return;
	}

	if (!Buffer->ConsumeBufferedInputSince(EInputCommandType::LightAttack, AbilityActivationTime))
	{
		return;
	}

	Buffer->ClearBuffer();
	const bool bActivated = PlayerOwner->ComboRuntimeComponent->HasComboSource()
		&& PlayerOwner->ComboRuntimeComponent->TryActivateCombo(ECardRequiredAction::Light, PlayerOwner);

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

void UGA_PlayerSpecialAttack::ApplyDamageFromEvent(FGameplayTag EventTag, const FGameplayEventData& EventData)
{
	if (!bApplyDamageFromMontageEvents || bIsHandlingSpecialAttackEvent)
	{
		return;
	}

	TGuardValue<bool> ReentrancyGuard(bIsHandlingSpecialAttackEvent, true);

	const FYogGameplayEffectContainerSpec ContainerSpec = MakeEffectContainerSpec(EventTag, EventData, -1);
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

	TArray<AActor*> HitActors;
	CollectHitActors(ContainerSpec, HitActors);

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
