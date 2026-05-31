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
#include "Component/ComboRuntimeComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Data/GameplayAbilityComboGraph.h"
#include "Data/MontageConfigDA.h"
#include "Engine/World.h"

namespace
{
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

	float FrameToMontageTime(int32 Frame, int32 TotalFrames, const UAnimMontage* Montage)
	{
		const float Duration = Montage ? Montage->GetPlayLength() : 0.f;
		const float Normalized = TotalFrames > 0
			? FMath::Clamp(static_cast<float>(Frame) / static_cast<float>(TotalFrames), 0.f, 1.f)
			: 0.f;
		return Normalized * Duration;
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

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ComboWindowOpenHandle);
		World->GetTimerManager().ClearTimer(ComboWindowCloseHandle);
	}

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		return;
	}

	UYogAbilitySystemComponent* ASC = Cast<UYogAbilitySystemComponent>(ActorInfo->AbilitySystemComponent);
	AYogCharacterBase* Owner = Cast<AYogCharacterBase>(ActorInfo->AvatarActor.Get());
	APlayerCharacterBase* PlayerOwner = Cast<APlayerCharacterBase>(Owner);
	UComboRuntimeComponent* ComboRuntime = PlayerOwner ? PlayerOwner->ComboRuntimeComponent.Get() : nullptr;
	const FWeaponComboNodeConfig* ActiveComboNode = ComboRuntime ? ComboRuntime->GetActiveNode() : nullptr;

	UAnimMontage* MontageToPlay = nullptr;
	if (ActiveComboNode)
	{
		MontageToPlay = ActiveComboNode->Montage;
		if (!MontageToPlay && ActiveComboNode->MontageConfig)
		{
			MontageToPlay = ActiveComboNode->MontageConfig->Montage;
		}
	}

	if (!ComboRuntime || !ActiveComboNode)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GA_PlayMontage] No active combo node on owner=%s."), *GetNameSafe(Owner));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!MontageToPlay)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GA_PlayMontage] Active combo node has no Montage on owner=%s."), *GetNameSafe(Owner));
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

	if (ActiveComboNode && ActiveComboNode->bOverrideComboWindow && ActiveComboNode->ComboWindowTotalFrames > 0)
	{
		const float StartTime = FrameToMontageTime(ActiveComboNode->ComboWindowStartFrame, ActiveComboNode->ComboWindowTotalFrames, MontageToPlay);
		const float EndTime = FrameToMontageTime(ActiveComboNode->ComboWindowEndFrame, ActiveComboNode->ComboWindowTotalFrames, MontageToPlay);
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(ComboWindowOpenHandle, this, &UGA_PlayMontage::OnComboWindowOpen, FMath::Max(StartTime, 0.01f), false);
			World->GetTimerManager().SetTimer(ComboWindowCloseHandle, this, &UGA_PlayMontage::OnComboWindowClose, FMath::Max(EndTime, 0.01f), false);
		}
	}

}

void UGA_PlayMontage::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	ActivePlayMontageTask = nullptr;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ComboWindowOpenHandle);
		World->GetTimerManager().ClearTimer(ComboWindowCloseHandle);
	}
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
	ResetComboToRoot();
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
}

void UGA_PlayMontage::OnMontageBlendOut()
{
	ResetComboToRoot();
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
}

void UGA_PlayMontage::OnMontageInterrupted()
{
	ResetComboToRoot();
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
}

void UGA_PlayMontage::OnMontageCancelled()
{
	ResetComboToRoot();
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, true);
}

void UGA_PlayMontage::ResetComboToRoot()
{
	if (APlayerCharacterBase* PlayerOwner = Cast<APlayerCharacterBase>(GetAvatarActorFromActorInfo()))
	{
		if (PlayerOwner->ComboRuntimeComponent)
		{
			PlayerOwner->ComboRuntimeComponent->ResetCombo();
		}
	}
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
		CollectHitActors(ContainerSpec, HitActors);

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

void UGA_PlayMontage::OnComboWindowOpen()
{
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->AddLooseGameplayTag(FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.CanCombo")));
	}
}

void UGA_PlayMontage::OnComboWindowClose()
{
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->SetLooseGameplayTagCount(FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.CanCombo")), 0);
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

	EInputCommandType BufferedAttackType = EInputCommandType::LightAttack;
	if (Buffer->ConsumeLatestAttackInputSince(AbilityActivationTime, BufferedAttackType))
	{
		Buffer->ClearBuffer();
		bool bActivated = false;
		bool bHasComboSource = false;
		const ECardRequiredAction ActionType = BufferedAttackType == EInputCommandType::HeavyAttack
			? ECardRequiredAction::Heavy
			: ECardRequiredAction::Light;
		const TCHAR* FallbackTagName = BufferedAttackType == EInputCommandType::HeavyAttack
			? TEXT("PlayerState.AbilityCast.HeavyAtk")
			: TEXT("PlayerState.AbilityCast.LightAtk");
		if (APlayerCharacterBase* PlayerOwner = Cast<APlayerCharacterBase>(Owner))
		{
			bHasComboSource = PlayerOwner->ComboRuntimeComponent && PlayerOwner->ComboRuntimeComponent->HasComboSource();
			bActivated = bHasComboSource
				&& PlayerOwner->ComboRuntimeComponent->TryActivateCombo(ActionType, PlayerOwner);
		}
		if (!bActivated && !bHasComboSource)
		{
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

	if (Buffer->HasBufferedInputSince(EInputCommandType::Dash, AbilityActivationTime))
	{
		Buffer->ClearBuffer();
		bool bActivated = false;
		if (APlayerCharacterBase* PlayerOwner = Cast<APlayerCharacterBase>(Owner))
		{
			bActivated = PlayerOwner->ComboRuntimeComponent
				&& PlayerOwner->ComboRuntimeComponent->HasComboSource()
				&& PlayerOwner->ComboRuntimeComponent->TryActivateDash(PlayerOwner);
		}
		if (!bActivated && ASC)
		{
			ASC->SetLooseGameplayTagCount(CanComboTag, 0);
		}
	}
}
