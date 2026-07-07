// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/AbilityTask/YogTask_PlayMontageAbility.h"


#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "GameFramework/Character.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystemLog.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"

namespace
{
	// Optional montage-authored float curve that scales play rate over the
	// montage timeline. If the montage has no curve with this name, play rate
	// is left at the requested Rate (default 1.0).
	const FName YogMontage_PlayRateCurveName(TEXT("PlayRate"));
}

UYogTask_PlayMontageAbility::UYogTask_PlayMontageAbility(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Rate = 1.f;
	bStopWhenAbilityEnds = true;
	bTickingTask = true;
}

void UYogTask_PlayMontageAbility::TickTask(float DeltaTime)
{
	Super::TickTask(DeltaTime);

	if (!MontageToPlay || !MontageToPlay->HasCurveData(YogMontage_PlayRateCurveName))
	{
		return;
	}

	const FGameplayAbilityActorInfo* ActorInfo = Ability ? Ability->GetCurrentActorInfo() : nullptr;
	UAnimInstance* AnimInstance = ActorInfo ? ActorInfo->GetAnimInstance() : nullptr;
	if (!AnimInstance || !AnimInstance->Montage_IsPlaying(MontageToPlay))
	{
		return;
	}

	const float Position = AnimInstance->Montage_GetPosition(MontageToPlay);
	const float CurveValue = MontageToPlay->EvaluateCurveData(
		YogMontage_PlayRateCurveName, FAnimExtractContext(static_cast<double>(Position)), false);
	AnimInstance->Montage_SetPlayRate(MontageToPlay, FMath::Max(Rate * CurveValue, 0.01f));
}

UYogAbilitySystemComponent* UYogTask_PlayMontageAbility::GetTargetASC()
{
	return Cast<UYogAbilitySystemComponent>(AbilitySystemComponent);
}

void UYogTask_PlayMontageAbility::OnMontageBlendingOut(UAnimMontage* Montage, bool bInterrupted)
{
	if (Ability && Ability->GetCurrentMontage() == MontageToPlay)
	{
		if (Montage == MontageToPlay)
		{
			AbilitySystemComponent->ClearAnimatingAbility(Ability);

			// Reset AnimRootMotionTranslationScale
			ACharacter* Character = Cast<ACharacter>(GetAvatarActor());
			if (Character && (Character->GetLocalRole() == ROLE_Authority ||
				(Character->GetLocalRole() == ROLE_AutonomousProxy && Ability->GetNetExecutionPolicy() == EGameplayAbilityNetExecutionPolicy::LocalPredicted)))
			{
				Character->SetAnimRootMotionTranslationScale(1.f);
			}

		}
	}

	if (bInterrupted)
	{
		if (ShouldBroadcastAbilityTaskDelegates())
		{
			OnInterrupted.Broadcast();
		}
	}
	else
	{
		if (ShouldBroadcastAbilityTaskDelegates())
		{
			OnBlendOut.Broadcast();
		}
	}
}

void UYogTask_PlayMontageAbility::OnAbilityCancelled()
{
	// TODO: Merge this fix back to engine, it was calling the wrong callback

	if (StopPlayingMontage())
	{
		// Let the BP handle the interrupt as well
		if (ShouldBroadcastAbilityTaskDelegates())
		{
			OnCancelled.Broadcast();
		}
	}
}

void UYogTask_PlayMontageAbility::OnMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (!bInterrupted)
	{
		if (ShouldBroadcastAbilityTaskDelegates())
		{
			OnCompleted.Broadcast();
		}
	}

	EndTask();
}

void UYogTask_PlayMontageAbility::OnGameplayEvent(FGameplayTag EventTag, const FGameplayEventData* Payload)
{
	// Universal re-entry guard: any listener (C++, BP, plugin) that calls
	// SendGameplayEvent inside its handler would otherwise loop back here.
	// This makes the multicast non-reentrant regardless of who's bound.
	if (bIsBroadcastingEvent)
	{
		UE_LOG(LogTemp, Warning, TEXT("[YogTask_PlayMontageAbility] Suppressed recursive OnGameplayEvent for tag %s"), *EventTag.ToString());
		return;
	}

	if (ShouldBroadcastAbilityTaskDelegates())
	{
		FGameplayEventData TempData = *Payload;
		TempData.EventTag = EventTag;

		TGuardValue<bool> ReentrancyGuard(bIsBroadcastingEvent, true);
		OnEventReceived.Broadcast(EventTag, TempData);
	}
}

UYogTask_PlayMontageAbility* UYogTask_PlayMontageAbility::YogPlayMontageAbility(UGameplayAbility* OwningAbility,
	FName TaskInstanceName, UAnimMontage* MontageToPlay, FGameplayTagContainer EventTags, float Rate, FName StartSection, bool bStopWhenAbilityEnds, float AnimRootMotionTranslationScale)
{
	UAbilitySystemGlobals::NonShipping_ApplyGlobalAbilityScaler_Rate(Rate);

	UYogTask_PlayMontageAbility* MyObj = NewAbilityTask<UYogTask_PlayMontageAbility>(OwningAbility, TaskInstanceName);
	MyObj->MontageToPlay = MontageToPlay;
	MyObj->EventTags = EventTags;
	MyObj->Rate = Rate;
	MyObj->StartSection = StartSection;
	MyObj->AnimRootMotionTranslationScale = AnimRootMotionTranslationScale;
	MyObj->bStopWhenAbilityEnds = bStopWhenAbilityEnds;

	return MyObj;
}

void UYogTask_PlayMontageAbility::Activate()
{
	if (Ability == nullptr)
	{
		return;
	}

	bool bPlayedMontage = false;
	UYogAbilitySystemComponent* YogAbilitySystemComponent = GetTargetASC();

	if (YogAbilitySystemComponent)
	{
		const FGameplayAbilityActorInfo* ActorInfo = Ability->GetCurrentActorInfo();
		UAnimInstance* AnimInstance = ActorInfo->GetAnimInstance();
		if (AnimInstance != nullptr)
		{
			// Bind to event callback
			EventHandle = YogAbilitySystemComponent->AddGameplayEventTagContainerDelegate(EventTags, FGameplayEventTagMulticastDelegate::FDelegate::CreateUObject(this, &UYogTask_PlayMontageAbility::OnGameplayEvent));

			if (YogAbilitySystemComponent->PlayMontage(Ability, Ability->GetCurrentActivationInfo(), MontageToPlay, Rate, StartSection) > 0.f)
			{
				// Playing a montage could potentially fire off a callback into game code which could kill this ability! Early out if we are  pending kill.
				if (ShouldBroadcastAbilityTaskDelegates() == false)
				{
					return;
				}

				CancelledHandle = Ability->OnGameplayAbilityCancelled.AddUObject(this, &UYogTask_PlayMontageAbility::OnAbilityCancelled);

				BlendingOutDelegate.BindUObject(this, &UYogTask_PlayMontageAbility::OnMontageBlendingOut);
				AnimInstance->Montage_SetBlendingOutDelegate(BlendingOutDelegate, MontageToPlay);

				MontageEndedDelegate.BindUObject(this, &UYogTask_PlayMontageAbility::OnMontageEnded);
				AnimInstance->Montage_SetEndDelegate(MontageEndedDelegate, MontageToPlay);

				ACharacter* Character = Cast<ACharacter>(GetAvatarActor());
				if (Character && (Character->GetLocalRole() == ROLE_Authority ||
					(Character->GetLocalRole() == ROLE_AutonomousProxy && Ability->GetNetExecutionPolicy() == EGameplayAbilityNetExecutionPolicy::LocalPredicted)))
				{
					Character->SetAnimRootMotionTranslationScale(AnimRootMotionTranslationScale);
				}

				bPlayedMontage = true;
			}
		}
		else
		{
			ABILITY_LOG(Warning, TEXT("UYogTask_PlayMontageAbility call to PlayMontage failed!"));
		}
	}
	else
	{
		ABILITY_LOG(Warning, TEXT("UYogTask_PlayMontageAbility called on invalid AbilitySystemComponent"));
	}

	if (!bPlayedMontage)
	{
		ABILITY_LOG(Warning, TEXT("UYogTask_PlayMontageAbility called in Ability %s failed to play montage %s; Task Instance Name %s."), *Ability->GetName(), *GetNameSafe(MontageToPlay), *InstanceName.ToString());
		if (ShouldBroadcastAbilityTaskDelegates())
		{
			OnCancelled.Broadcast();
		}
	}

	SetWaitingOnAvatar();
}

void UYogTask_PlayMontageAbility::ExternalCancel()
{
	UAbilitySystemComponent* ASC = GetTargetASC();

	if (ASC)
		//check(AbilitySystemComponent);

		OnAbilityCancelled();

	Super::ExternalCancel();
}

void UYogTask_PlayMontageAbility::OnDestroy(bool AbilityEnded)
{
	// Note: Clearing montage end delegate isn't necessary since its not a multicast and will be cleared when the next montage plays.
	// (If we are destroyed, it will detect this and not do anything)

	// This delegate, however, should be cleared as it is a multicast
	if (Ability)
	{
		Ability->OnGameplayAbilityCancelled.Remove(CancelledHandle);
		if (AbilityEnded && bStopWhenAbilityEnds)
		{
			StopPlayingMontage();
		}
	}

	UYogAbilitySystemComponent* YogAbilitySystemComponent = GetTargetASC();
	if (YogAbilitySystemComponent)
	{
		YogAbilitySystemComponent->RemoveGameplayEventTagContainerDelegate(EventTags, EventHandle);
	}

	Super::OnDestroy(AbilityEnded);

}

bool UYogTask_PlayMontageAbility::StopPlayingMontage()
{
	const FGameplayAbilityActorInfo* ActorInfo = Ability->GetCurrentActorInfo();
	if (!ActorInfo)
	{
		return false;
	}

	UAnimInstance* AnimInstance = ActorInfo->GetAnimInstance();
	if (AnimInstance == nullptr)
	{
		return false;
	}

	// Check if the montage is still playing
	// The ability would have been interrupted, in which case we should automatically stop the montage
	UAbilitySystemComponent* ASC = GetTargetASC();

	if (ASC && Ability)
		//if (AbilitySystemComponent && Ability)
	{
		if (AbilitySystemComponent->GetAnimatingAbility() == Ability
			&& AbilitySystemComponent->GetCurrentMontage() == MontageToPlay)
		{
			// Unbind delegates so they don't get called as well
			FAnimMontageInstance* MontageInstance = AnimInstance->GetActiveInstanceForMontage(MontageToPlay);
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

FString UYogTask_PlayMontageAbility::GetDebugString() const
{
	UAnimMontage* PlayingMontage = nullptr;
	if (Ability)
	{
		const FGameplayAbilityActorInfo* ActorInfo = Ability->GetCurrentActorInfo();
		UAnimInstance* AnimInstance = ActorInfo->GetAnimInstance();

		if (AnimInstance != nullptr)
		{
			PlayingMontage = AnimInstance->Montage_IsActive(MontageToPlay) ? MontageToPlay : AnimInstance->GetCurrentActiveMontage();
		}
	}

	return FString::Printf(TEXT("PlayMontageAndWaitForEvent. MontageToPlay: %s  (Currently Playing): %s"), *GetNameSafe(MontageToPlay), *GetNameSafe(PlayingMontage));
}
