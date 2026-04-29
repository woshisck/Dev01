// Copyright 2022 Mickael Daniel. All Rights Reserved.


#include "Abilities/Tasks/ComboGraphAbilityTask_PlayMontage.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "ComboGraphLog.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "Runtime/Launch/Resources/Version.h"

UComboGraphAbilityTask_PlayMontage::UComboGraphAbilityTask_PlayMontage(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bTickingTask = true;
}

void UComboGraphAbilityTask_PlayMontage::Activate()
{
	if (!Ability)
	{
		return;
	}

	SetWaitingOnAvatar();

	CreateAutoSetupNotifyStates();

	bool bPlayedMontage = false;

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
	if (AbilitySystemComponent.IsValid())
#else
	if (AbilitySystemComponent)
#endif
	{
		const FGameplayAbilityActorInfo* ActorInfo = Ability->GetCurrentActorInfo();
		CachedAnimInstance = ActorInfo ? ActorInfo->GetAnimInstance() : nullptr;

		if (CachedAnimInstance != nullptr)
		{
			// Bind to event callback
			EventHandle = AbilitySystemComponent->AddGameplayEventTagContainerDelegate(EventTags, FGameplayEventTagMulticastDelegate::FDelegate::CreateUObject(this, &UComboGraphAbilityTask_PlayMontage::OnGameplayEvent));

			// float CurrentMontageSectionTimeLeft = AbilitySystemComponent->GetCurrentMontageSectionTimeLeft();
			if (AbilitySystemComponent->PlayMontage(Ability, Ability->GetCurrentActivationInfo(), MontageToPlay, Rate, StartSection) > 0.f)
			{
				// Playing a montage could potentially fire off a callback into game code which could kill this ability! Early out if we are  pending kill.
				if (ShouldBroadcastAbilityTaskDelegates() == false)
				{
					return;
				}

				CancelledHandle = Ability->OnGameplayAbilityCancelled.AddUObject(this, &UComboGraphAbilityTask_PlayMontage::OnAbilityCancelled);

				BlendingOutDelegate.BindUObject(this, &UComboGraphAbilityTask_PlayMontage::OnMontageBlendingOut);
				CachedAnimInstance->Montage_SetBlendingOutDelegate(BlendingOutDelegate, MontageToPlay);

				MontageEndedDelegate.BindUObject(this, &UComboGraphAbilityTask_PlayMontage::OnMontageEnded);
				CachedAnimInstance->Montage_SetEndDelegate(MontageEndedDelegate, MontageToPlay);

				ACharacter* Character = Cast<ACharacter>(GetAvatarActor());
				if (Character && (Character->GetLocalRole() == ROLE_Authority || (Character->GetLocalRole() == ROLE_AutonomousProxy && Ability->GetNetExecutionPolicy() == EGameplayAbilityNetExecutionPolicy::LocalPredicted)))
				{
					Character->SetAnimRootMotionTranslationScale(AnimRootMotionTranslationScale);
				}

				bPlayedMontage = true;
			}
		}
		else
		{
			CG_RUNTIME_LOG(Warning, TEXT("UComboGraphAbilityTask_PlayMontage call to PlayMontage failed!"));
		}
	}
	else
	{
		CG_RUNTIME_LOG(Warning, TEXT("UComboGraphAbilityTask_PlayMontage called on invalid AbilitySystemComponent"));
	}

	if (!bPlayedMontage)
	{
		CG_RUNTIME_LOG(Warning, TEXT("UComboGraphAbilityTask_PlayMontage called in Ability %s failed to play montage %s; Task Instance Name %s."), *Ability->GetName(), *GetNameSafe(MontageToPlay),*InstanceName.ToString());
		if (ShouldBroadcastAbilityTaskDelegates())
		{
			OnCancelled.Broadcast(FGameplayTag(), FGameplayEventData());
		}
	}

}

void UComboGraphAbilityTask_PlayMontage::ExternalCancel()
{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
	check(AbilitySystemComponent.IsValid());
#else
	check(AbilitySystemComponent);
#endif
	OnAbilityCancelled();
	Super::ExternalCancel();
}

FString UComboGraphAbilityTask_PlayMontage::GetDebugString() const
{
	const UAnimMontage* PlayingMontage = nullptr;
	if (Ability && CachedAnimInstance)
	{
		PlayingMontage = CachedAnimInstance->Montage_IsActive(MontageToPlay) ? MontageToPlay.Get() : CachedAnimInstance->GetCurrentActiveMontage();
	}

	return FString::Printf(TEXT("%s (Montage: %s)"), *GetName(), *GetNameSafe(PlayingMontage));
}

void UComboGraphAbilityTask_PlayMontage::OnDestroy(bool bAbilityEnded)
{
	// Note: Clearing montage end delegate isn't necessary since its not a multicast and will be cleared when the next montage plays.
	// (If we are destroyed, it will detect this and not do anything)

	// This delegate, however, should be cleared as it is a multicast
	if (Ability)
	{
		Ability->OnGameplayAbilityCancelled.Remove(CancelledHandle);
		if (bAbilityEnded && bStopWhenAbilityEnds)
		{
			StopPlayingMontage();
		}
	}
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
	if (AbilitySystemComponent.IsValid())
#else
	if (AbilitySystemComponent)
#endif
	{
		AbilitySystemComponent->RemoveGameplayEventTagContainerDelegate(EventTags, EventHandle);
	}

	Super::OnDestroy(bAbilityEnded);
}

void UComboGraphAbilityTask_PlayMontage::TickTask(const float DeltaTime)
{
	Super::TickTask(DeltaTime);

	ElapsedTime = ElapsedTime + DeltaTime;

	if (!Ability)
	{
		return;
	}

	const FName CurrentSection = CachedAnimInstance ? CachedAnimInstance->Montage_GetCurrentSection(MontageToPlay) : NAME_None;

	if (CurrentSection != CachedCurrentSection)
	{
		CachedCurrentSection = CurrentSection;
		if (CachedCurrentSection != NAME_None)
		{
			CurrentSectionElapsedTime = 0.f;

			float StartTime, EndTime;
			MontageToPlay->GetSectionStartAndEndTime(MontageToPlay->GetSectionIndex(CurrentSection), StartTime, EndTime);
			CachedSectionsLength = EndTime - StartTime;
			OnSectionChanged.Broadcast(CurrentSection);

			// Reset begin / end state to consider them again for next section.
			TMap<TObjectPtr<UClass>, FComboGraphRegisteredNotifyData> NewRegisteredNotifyData;
			for (const auto& NotifyData : RegisteredNotifyData)
			{

				UClass* NotifyClass = NotifyData.Key;
				FComboGraphRegisteredNotifyData Data = NotifyData.Value;
				Data.ResetState();
				NewRegisteredNotifyData.Add(NotifyClass, Data);
			}

			RegisteredNotifyData.Reset();
			RegisteredNotifyData.Append(NewRegisteredNotifyData);
		}
	}

	CurrentSectionElapsedTime = CurrentSectionElapsedTime + DeltaTime;

	if (ShouldHandleAutoSetup())
	{
		USkeletalMeshComponent* SkeletalMeshComponent = Ability->GetOwningComponentFromActorInfo();
		float SequenceLength = CachedSectionsLength > 0.f ? CachedSectionsLength : MontageToPlay->GetPlayLength();

		if (Rate != 0)
		{
			// Adjust sequence length based on montage play rate and avoid division by 0
			SequenceLength = SequenceLength * (1 / Rate);
		}

		if (MontageToPlay->RateScale != 0)
		{
			// Adjust sequence length based on montage play rate and avoid division by 0 (this one is for the rate scale on the anim asset itself)
			SequenceLength = SequenceLength * (1 / MontageToPlay->RateScale);
		}

		for (UAnimNotifyState* NotifyState : RegisteredNotifyStates)
		{
			TickHandleNotifyState(SkeletalMeshComponent, SequenceLength, NotifyState);
		}
	}

	if (ShouldTriggerTransition())
	{
		bTransitionWasTriggered = true;
		OnTriggerTransition.Broadcast();
	}
}

void UComboGraphAbilityTask_PlayMontage::TickHandleNotifyState(USkeletalMeshComponent* SkeletalMeshComponent, const float SequenceLength, UAnimNotifyState* NotifyState)
{
	if (!NotifyState)
	{
		return;
	}


	FComboGraphRegisteredNotifyData* NotifyData = RegisteredNotifyData.Find(NotifyState->GetClass());
	if (!NotifyData)
	{
		return;
	}

	const FComboGraphNotifyStateAutoSetup AutoSetup = NotifyData->AutoSetup;

	const float StartTime = SequenceLength * AutoSetup.StartPercent;
	const float EndTime = SequenceLength * AutoSetup.EndPercent;
	const float TotalDuration = EndTime - StartTime;

	if (CurrentSectionElapsedTime >= StartTime && !NotifyData->bBeginNotified)
	{
		NotifyData->bBeginNotified = true;
		CG_RUNTIME_LOG(Verbose, TEXT("UComboGraphAbilityTask_PlayMontage - TickTask Should Trigger NotifyBegin - ElapsedTime >= StartTime - %f >= %f (%f)"), CurrentSectionElapsedTime, StartTime, SequenceLength);
		CG_RUNTIME_LOG(Verbose, TEXT("UComboGraphAbilityTask_PlayMontage - TickTask, Trigger notify begin on %s with skel mesh: %s"), *NotifyState->GetName(), SkeletalMeshComponent ? *SkeletalMeshComponent->GetName() : TEXT("NONE"));
		#if ENGINE_MAJOR_VERSION == 5
		NotifyState->NotifyBegin(SkeletalMeshComponent, MontageToPlay, TotalDuration, FAnimNotifyEventReference());
		#else
		NotifyState->NotifyBegin(SkeletalMeshComponent, MontageToPlay, TotalDuration);
		#endif
	}
	else if (CurrentSectionElapsedTime >= EndTime && !NotifyData->bEndNotified)
	{
		NotifyData->bEndNotified = true;
		CG_RUNTIME_LOG(Verbose, TEXT("UComboGraphAbilityTask_PlayMontage - TickTask Should Trigger NotifyEnd - ElapsedTime >= StartTime - %f >= %f (%f)"), CurrentSectionElapsedTime, EndTime, SequenceLength);
		CG_RUNTIME_LOG(Verbose, TEXT("UComboGraphAbilityTask_PlayMontage - TickTask, Trigger notify end on %s with skel mesh: %s"), *NotifyState->GetName(), SkeletalMeshComponent ? *SkeletalMeshComponent->GetName() : TEXT("NONE"));
		#if ENGINE_MAJOR_VERSION == 5
		NotifyState->NotifyEnd(SkeletalMeshComponent, MontageToPlay, FAnimNotifyEventReference());
		#else
		NotifyState->NotifyEnd(SkeletalMeshComponent, MontageToPlay);
		#endif
	}
}

bool UComboGraphAbilityTask_PlayMontage::ShouldTriggerTransition() const
{
	return !bTransitionWasTriggered && TransitionTriggerTime > 0 && ElapsedTime >= TransitionTriggerTime;
}

bool UComboGraphAbilityTask_PlayMontage::ShouldHandleAutoSetup() const
{
	return bHasNotifyAutoSetup;
}

void UComboGraphAbilityTask_PlayMontage::UnbindAllDelegate()
{
	OnCompleted.Clear();
	OnBlendOut.Clear();
	OnInterrupted.Clear();
	OnTriggerTransition.Clear();
	OnSectionChanged.Clear();
}

UComboGraphAbilityTask_PlayMontage* UComboGraphAbilityTask_PlayMontage::PlayComboGraphMontage(UGameplayAbility* OwningAbility, const FName TaskInstanceName, UAnimMontage* MontageToPlay, TMap<TSoftClassPtr<UAnimNotifyState>, FComboGraphNotifyStateAutoSetup> NotifiesAutoSetup, const FGameplayTagContainer EventTags, float Rate, const FName StartSection, const bool bStopWhenAbilityEnds, const float AnimRootMotionTranslationScale)
{
	UAbilitySystemGlobals::NonShipping_ApplyGlobalAbilityScaler_Rate(Rate);

	if (!OwningAbility)
	{
		// Prevent crash if two abilities bound to input action.
		// TODO: Investigate
		CG_RUNTIME_LOG(Error, TEXT("UComboGraphAbilityTask_PlayMontage - Owning Ability is null. This should never happen."));
		return nullptr;
	}

	UComboGraphAbilityTask_PlayMontage* Task = NewAbilityTask<UComboGraphAbilityTask_PlayMontage>(OwningAbility, TaskInstanceName);
	Task->MontageToPlay = MontageToPlay;
	Task->EventTags = EventTags;
	Task->Rate = Rate;
	Task->StartSection = StartSection;
	Task->AnimRootMotionTranslationScale = AnimRootMotionTranslationScale;
	Task->bStopWhenAbilityEnds = bStopWhenAbilityEnds;
	Task->NotifiesAutoSetup = NotifiesAutoSetup;

	return Task;
}

float UComboGraphAbilityTask_PlayMontage::GetElapsedTime() const
{
	return ElapsedTime;
}

void UComboGraphAbilityTask_PlayMontage::SetTransitionTriggerTime(const float InTriggerTime)
{
	TransitionTriggerTime = InTriggerTime;
}

bool UComboGraphAbilityTask_PlayMontage::StopPlayingMontage() const
{
	const FGameplayAbilityActorInfo* ActorInfo = Ability->GetCurrentActorInfo();
	if (!ActorInfo)
	{
		return false;
	}

	if (CachedAnimInstance == nullptr)
	{
		return false;
	}

	// Check if the montage is still playing
	// The ability would have been interrupted, in which case we should automatically stop the montage
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
	if (AbilitySystemComponent.IsValid() && Ability)
#else
	if (AbilitySystemComponent && Ability)
#endif
	{
		if (AbilitySystemComponent->GetAnimatingAbility() == Ability && AbilitySystemComponent->GetCurrentMontage() == MontageToPlay)
		{
			// Unbind delegates so they don't get called as well
			FAnimMontageInstance* MontageInstance = CachedAnimInstance->GetActiveInstanceForMontage(MontageToPlay);
			if (MontageInstance)
			{
				MontageInstance->OnMontageBlendingOutStarted.Unbind();
				MontageInstance->OnMontageEnded.Unbind();
			}

			AbilitySystemComponent->CurrentMontageStop();
			return true;
		}
	}

	return false;
}

// ReSharper disable once CppParameterMayBeConstPtrOrRef
void UComboGraphAbilityTask_PlayMontage::OnMontageBlendingOut(UAnimMontage* Montage, const bool bInterrupted) const
{
	if (Ability && Ability->GetCurrentMontage() == MontageToPlay)
	{
		if (Montage == MontageToPlay)
		{
			AbilitySystemComponent->ClearAnimatingAbility(Ability);

			// Reset AnimRootMotionTranslationScale
			ACharacter* Character = Cast<ACharacter>(GetAvatarActor());
			if (Character && (Character->GetLocalRole() == ROLE_Authority || (Character->GetLocalRole() == ROLE_AutonomousProxy && Ability->GetNetExecutionPolicy() == EGameplayAbilityNetExecutionPolicy::LocalPredicted)))
			{
				Character->SetAnimRootMotionTranslationScale(1.f);
			}

		}
	}

	if (bInterrupted)
	{
		if (ShouldBroadcastAbilityTaskDelegates())
		{
			OnInterrupted.Broadcast(FGameplayTag(), FGameplayEventData());
		}
	}
	else
	{
		if (ShouldBroadcastAbilityTaskDelegates())
		{
			OnBlendOut.Broadcast(FGameplayTag(), FGameplayEventData());
		}
	}
}

void UComboGraphAbilityTask_PlayMontage::OnAbilityCancelled() const
{
	if (StopPlayingMontage())
	{
		// Let the BP handle the interrupt as well
		if (ShouldBroadcastAbilityTaskDelegates())
		{
			OnCancelled.Broadcast(FGameplayTag(), FGameplayEventData());
		}
	}
}

void UComboGraphAbilityTask_PlayMontage::OnMontageEnded(UAnimMontage* Montage, const bool bInterrupted)
{
	if (ShouldBroadcastAbilityTaskDelegates())
	{
		if (bInterrupted)
		{
			OnInterrupted.Broadcast(FGameplayTag(), FGameplayEventData());
		}
		else
		{
			OnCompleted.Broadcast(FGameplayTag(), FGameplayEventData());
		}
	}

	EndTask();
}

void UComboGraphAbilityTask_PlayMontage::OnGameplayEvent(const FGameplayTag EventTag, const FGameplayEventData* Payload) const
{
	if (ShouldBroadcastAbilityTaskDelegates())
	{
		FGameplayEventData TempData = *Payload;
		TempData.EventTag = EventTag;

		EventReceived.Broadcast(EventTag, TempData);
	}
}

void UComboGraphAbilityTask_PlayMontage::CreateAutoSetupNotifyStates()
{
	for (const TPair<TSoftClassPtr<UAnimNotifyState>, FComboGraphNotifyStateAutoSetup>& AutoSetup : NotifiesAutoSetup)
	{
		TSoftClassPtr<UAnimNotifyState> NotifyClassPtr = AutoSetup.Key;
		const FComboGraphNotifyStateAutoSetup NotifyStateAutoSetup = AutoSetup.Value;
		if (!NotifyClassPtr.IsNull())
		{
			UClass* NotifyClass = NotifyClassPtr.LoadSynchronous();
			if (!HasAnimNotify(MontageToPlay, NotifyClass))
			{
				// Montage doesn't have this auto setup Notify State, create and store for later activation on tick
				UAnimNotifyState* AnimNotify = NewObject<UAnimNotifyState>(MontageToPlay, NotifyClass, NAME_None, RF_Transactional);
				if (AnimNotify)
				{
					RegisteredNotifyStates.AddUnique(AnimNotify);

					FComboGraphRegisteredNotifyData& NotifyData = RegisteredNotifyData.FindOrAdd(AnimNotify->GetClass());
					NotifyData.AutoSetup = NotifyStateAutoSetup;
				}
			}
		}
	}

	if (RegisteredNotifyStates.Num() > 0)
	{
		bHasNotifyAutoSetup = true;
	}
}

FAnimNotifyEvent* UComboGraphAbilityTask_PlayMontage::GetAnimNotify(UAnimSequenceBase* AnimSequenceBase, UClass* NotifyClass)
{
	check(AnimSequenceBase)
	return AnimSequenceBase->Notifies.FindByPredicate([NotifyClass](const FAnimNotifyEvent NotifyEvent) -> bool
	{
		return NotifyEvent.NotifyStateClass ? NotifyEvent.NotifyStateClass->GetClass() == NotifyClass : false;
	});
}

bool UComboGraphAbilityTask_PlayMontage::HasAnimNotify(UAnimMontage* Montage, UClass* NotifyClass)
{
	check(Montage);

	// Check if any anim segments in montage has a sequence with anim notify already
	for (FSlotAnimationTrack SlotAnimTrack : Montage->SlotAnimTracks)
	{
		for (const FAnimSegment AnimSegment : SlotAnimTrack.AnimTrack.AnimSegments)
		{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 1
			UAnimSequenceBase* AnimReference = AnimSegment.GetAnimReference();
#else
			UAnimSequenceBase* AnimReference = AnimSegment.AnimReference;
#endif
			if (!AnimReference)
			{
				CG_RUNTIME_LOG(Error, TEXT("UComboGraphAbilityTask_PlayMontage::HasAnimNotify() - It appears `%s` Montage has one of its anim reference set to none in one of its slot tracks."), *Montage->GetName())
				continue;
			}

			const FAnimNotifyEvent* NotifyEvent = GetAnimNotify(AnimReference, NotifyClass);
			if (NotifyEvent)
			{
				return true;
			}
		}
	}

	// Then check montage itself
	const FAnimNotifyEvent* NotifyEvent = GetAnimNotify(Montage, NotifyClass);
	return NotifyEvent ? true : false;
}
