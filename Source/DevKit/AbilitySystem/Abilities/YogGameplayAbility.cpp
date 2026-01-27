// Fill out your copyright notice in the Description page of Project Settings.


#include "YogGameplayAbility.h"
#include "YogTargetType.h"
#include "DevKit/Character/YogCharacterBase.h"
#include "../YogAbilitySystemComponent.h"
#include "Data/AbilityData.h"
#include "AbilitySystemComponent.h"

UYogGameplayAbility::UYogGameplayAbility(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	//NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
}

void UYogGameplayAbility::OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	Super::OnGiveAbility(ActorInfo, Spec);

	K2_OnAbilityAdded();
}

void UYogGameplayAbility::OnRemoveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	//if (UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get())
	//{
	//	// Remove delegate
	//	this->OnCooldownChange.Remove(CooldownChangeDelegateHandle);
	//}

	// Clear timer if using polling approach

	Super::OnRemoveAbility(ActorInfo, Spec);
	K2_OnAbilityRemoved();


}

void UYogGameplayAbility::OnAvatarSet(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	Super::OnAvatarSet(ActorInfo, Spec);

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC) return;


}



TArray<FActiveGameplayEffectHandle> UYogGameplayAbility::ApplyEffectContainer(FGameplayTag ContainerTag, const FGameplayEventData& EventData, int32 OverrideGameplayLevel )
{
	UE_LOG(LogTemp, Warning, TEXT("ApplyEffectContainer"));
	FYogGameplayEffectContainerSpec Spec = MakeEffectContainerSpec(ContainerTag, EventData, OverrideGameplayLevel);



	return ApplyEffectContainerSpec(Spec);
}

FGameplayEffectSpecHandle UYogGameplayAbility::AddGameplayCueParametersToSpec(const FGameplayEffectSpecHandle& OriginalSpec, const FGameplayCueParameters& CueParameters)
{
	if (!OriginalSpec.IsValid() || !OriginalSpec.Data.IsValid())
	{
		return OriginalSpec;
	}

	// Create a copy of the spec
	FGameplayEffectSpec* NewSpec = new FGameplayEffectSpec(*OriginalSpec.Data.Get());
	FGameplayEffectSpecHandle NewSpecHandle = FGameplayEffectSpecHandle(NewSpec);

	// Get the context and modify it
	FGameplayEffectContextHandle ContextHandle = NewSpec->GetContext();

	// Transfer CueParameters to the context
	if (CueParameters.EffectContext.IsValid())
	{
		ContextHandle = CueParameters.EffectContext;
	}
	else
	{
	}

	// Set the modified context back
	NewSpec->SetContext(ContextHandle);

	return NewSpecHandle;
}

float UYogGameplayAbility::GetRemainingCooldownTime() const
{
	// Check if the ability is active and has a valid actor info
	if (const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo())
	{
		if (UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get())
		{
			// Use the built-in GetCooldownTimeRemaining method
			return GetCooldownTimeRemaining();
		}
	}

	return 0.0f;
}

FString UYogGameplayAbility::GetRemainingCooldownTimeString() const
{
	float RemainingTime = GetRemainingCooldownTime();

	if (RemainingTime <= 0.0f)
	{
		return FString(TEXT("Ready"));
	}

	int32 Minutes = FMath::FloorToInt(RemainingTime / 60.0f);
	int32 Seconds = FMath::FloorToInt(FMath::Fmod(RemainingTime, 60.0f));

	return FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
}

bool UYogGameplayAbility::IsOnCooldown() const
{
	return GetRemainingCooldownTime() > 0.0f;
}


void UYogGameplayAbility::UpdateRetrigger(bool retriggerable)
{
	this->bRetriggerInstancedAbility = retriggerable;

}



AYogCharacterBase* UYogGameplayAbility::GetOwnerCharacterInfo()
{
	AYogCharacterBase* OwningCharacter = NewObject<AYogCharacterBase>(this, AYogCharacterBase::StaticClass());
	AActor* OwningActor = NewObject<AActor>(this, AActor::StaticClass());
	OwningActor = GetOwningActorFromActorInfo();

	if (OwningActor != NULL)
	{
		OwningCharacter = Cast<AYogCharacterBase>(OwningActor);
		return OwningCharacter;
	}
	else
	{
		return OwningCharacter;
	}

}


int UYogGameplayAbility::GetCurrentGameplayEffect(FGameplayTag EffectTag)
{
	int StackCount = 0;
	// Define the GameplayTag you want to check for active Gameplay Effects
	//FGameplayTag EffectTag = FGameplayTag::RequestGameplayTag(FName("Your.GameplayEffect.Tag"));

	// Prepare a container to hold the active effects
	TArray<FActiveGameplayEffectHandle> ActiveEffectHandles;

	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	AYogCharacterBase* player = Cast<AYogCharacterBase>(AvatarActor);
	UYogAbilitySystemComponent* asc = player->GetASC();

	// Get active effects that have this tag
	TArray<FActiveGameplayEffectHandle> GameplayEffectHandles = asc->GetActiveEffectsWithAllTags(FGameplayTagContainer(EffectTag));

	for (const FActiveGameplayEffectHandle& Handle : GameplayEffectHandles)
	{
		if (const FActiveGameplayEffect* ActiveEffect = asc->GetActiveGameplayEffect(Handle))
		{
			// You can access effect properties like Spec and StackCount here
			StackCount = ActiveEffect->Spec.GetStackCount();

			// Do something with the active effect
		}
	}
	return StackCount;
}







FYogGameplayEffectContainerSpec UYogGameplayAbility::MakeEffectContainerSpecFromContainer(const FYogGameplayEffectContainer& Container, const FGameplayEventData& EventData, int32 OverrideGameplayLevel)
{
	//TargetType collect data from world
	FYogGameplayEffectContainerSpec ReturnSpec;
	AActor* OwningActor = GetOwningActorFromActorInfo();
	AYogCharacterBase* OwningCharacter = Cast<AYogCharacterBase>(OwningActor);
	UYogAbilitySystemComponent* OwningASC = OwningCharacter->GetASC();

	if (OwningASC)
	{
		
		if (Container.TargetType.Get())
		{
			//@CAUTION GetTargets set HitResults and TargetActors from BP 
			TArray<FHitResult> HitResults;
			TArray<AActor*> TargetActors;


			const UYogTargetType* TargetTypeCDO = Container.TargetType.GetDefaultObject();
			AActor* AvatarActor = GetAvatarActorFromActorInfo();
			
			TargetTypeCDO->GetTargets(OwningCharacter, AvatarActor, EventData, HitResults, TargetActors);
			ReturnSpec.AddTargets(HitResults, TargetActors);
		}

		// If we don't have an override level, use the default on the ability itself
		if (OverrideGameplayLevel == INDEX_NONE)
		{
			OverrideGameplayLevel = this->GetAbilityLevel(); //OwningASC->GetDefaultAbilityLevel();
		}

		// Build GameplayEffectSpecs for each applied effect
		//for (const TSubclassOf<UGameplayEffect>& EffectClass : Container.EffectClasses)
		for (const FYogEffectPorperty& YogEffectProperty : Container.EffectClasses)
		{

			FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(YogEffectProperty.GameplayEffect, YogEffectProperty.EffectLevel);

			if (SpecHandle.IsValid() && SpecHandle.Data.IsValid())
			{
				FGameplayEffectContextHandle ContextHandle = SpecHandle.Data->GetContext();


				// OR: Create a modified context and set it back
				// FGameplayEffectContextHandle ContextHandle = SpecHandle.Data->GetContext();

				// Set context parameters
				ContextHandle.AddSourceObject(GetAvatarActorFromActorInfo());
				ContextHandle.AddInstigator(GetAvatarActorFromActorInfo(), GetAvatarActorFromActorInfo()->GetInstigatorController());
				ContextHandle.SetAbility(this);
				SpecHandle.Data->SetContext(ContextHandle);
			}


			ReturnSpec.TargetGameplayEffectSpecs.Add(SpecHandle);

			
		}
	}
	return ReturnSpec;
}


FYogGameplayEffectContainerSpec UYogGameplayAbility::MakeEffectContainerSpec(FGameplayTag ContainerTag, const FGameplayEventData& EventData, int32 OverrideGameplayLevel)
{
	AYogCharacterBase* OwningCharacter = GetOwnerCharacterInfo();

	//TODO: Find the gameplayeffect stack (card+ unique feature)
	UYogAbilitySystemComponent* ASC = OwningCharacter->GetASC();

	//Find the Container var in PLASYEBASE(Avatar), and find if tag exist in the buff
	FYogGameplayEffectContainer* FoundContainer = ASC->EffectContainerMap.Find(ContainerTag);


	if (FoundContainer)
	{
		return MakeEffectContainerSpecFromContainer(*FoundContainer, EventData, OverrideGameplayLevel);
	}
	return FYogGameplayEffectContainerSpec();
}




TArray<FActiveGameplayEffectHandle> UYogGameplayAbility::ApplyEffectContainerSpec(const FYogGameplayEffectContainerSpec& ContainerSpec)
{
	TArray<FActiveGameplayEffectHandle> AllEffects;

	// Iterate list of effect specs and apply them to their target data
	for (const FGameplayEffectSpecHandle& SpecHandle : ContainerSpec.TargetGameplayEffectSpecs)
	{
		FGameplayCueParameters CueParameters;

		FGameplayEffectSpecHandle ModifiedSpecHandle = AddGameplayCueParametersToSpec(
			SpecHandle,
			CueParameters
		);

		AllEffects.Append(K2_ApplyGameplayEffectSpecToTarget(SpecHandle, ContainerSpec.TargetData));
	}
	return AllEffects;
}


void UYogGameplayAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	EventOn_AbilityStart.Broadcast();
}

void UYogGameplayAbility::OnCooldownEffectAdded(UAbilitySystemComponent* TargetASC, const FGameplayEffectSpec& SpecApplied, FActiveGameplayEffectHandle ActiveHandle)
{
}

void UYogGameplayAbility::OnCooldownEffectRemoved(const FActiveGameplayEffect& EffectRemoved)
{
}




//TODO: NEED TO CHANGE FUNCTION NAME FOR FURTHER DIFFERENT 
FActionData UYogGameplayAbility::GetRowData(FDataTableRowHandle action_row)
{
	FActionData Result;
	if (!action_row.IsNull())
	{
		FActionData* actionData = action_row.GetRow<FActionData>(__func__);
		return *actionData;

	}
	return Result;
}

void UYogGameplayAbility::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);

	EventOn_AbilityEnded.Broadcast();
}



FGameplayTag UYogGameplayAbility::GetFirstTagFromContainer(const FGameplayTagContainer& Container)
{
	if (Container.IsEmpty()) return FGameplayTag::EmptyTag;

	// Get the first tag (order may not be guaranteed)
	return *Container.CreateConstIterator();
}

FGameplayTagContainer& UYogGameplayAbility::GetAbilityTags()
{
	return this->AbilityTags;
}

FGameplayTagContainer& UYogGameplayAbility::GetCancelAbilitiesWithTag()
{
	// TODO: insert return statement here
	return this->CancelAbilitiesWithTag;
}

FGameplayTagContainer& UYogGameplayAbility::GetBlockAbilitiesWithTag()
{
	// TODO: insert return statement here
	return this->BlockAbilitiesWithTag;
}

FGameplayTagContainer& UYogGameplayAbility::GetActivationOwnedTags()
{
	// TODO: insert return statement here
	return this->ActivationOwnedTags;
}

FGameplayTagContainer& UYogGameplayAbility::GetActivationRequiredTags()
{
	// TODO: insert return statement here
	return this->ActivationRequiredTags;
}

FGameplayTagContainer& UYogGameplayAbility::GetActivationBlockedTags()
{
	// TODO: insert return statement here
	return this->ActivationBlockedTags;
}

FGameplayTagContainer& UYogGameplayAbility::GetSourceRequiredTags()
{
	// TODO: insert return statement here
	return this->SourceRequiredTags;
}

FGameplayTagContainer& UYogGameplayAbility::GetSourceBlockedTags()
{
	// TODO: insert return statement here
	return this->SourceBlockedTags;
}

FGameplayTagContainer& UYogGameplayAbility::GetTargetRequiredTags()
{
	// TODO: insert return statement here
	return this->TargetRequiredTags;
}

FGameplayTagContainer& UYogGameplayAbility::GetTargetBlockedTags()
{
	// TODO: insert return statement here
	return TargetBlockedTags;
}
