// Fill out your copyright notice in the Description page of Project Settings.


#include "YogGameplayAbility.h"
#include "YogTargetType.h"
#include "../../Character/YogCharacterBase.h"
#include "../YogAbilitySystemComponent.h"


UYogGameplayAbility::UYogGameplayAbility(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

}



TArray<FActiveGameplayEffectHandle> UYogGameplayAbility::ApplyEffectContainer(FGameplayTag ContainerTag, const FGameplayEventData& EventData, int32 OverrideGameplayLevel )
{
	FYogGameplayEffectContainerSpec Spec = MakeEffectContainerSpec(ContainerTag, EventData, OverrideGameplayLevel);
	return ApplyEffectContainerSpec(Spec);
}


void UYogGameplayAbility::UpdateRetrigger(bool retriggerable)
{
	this->bRetriggerInstancedAbility = retriggerable;
}

void UYogGameplayAbility::GetAbilityTableData()
{
	static const FString ContextString(TEXT("Character Data Lookup"));
	FYogAbilityData* AbilityData = YogAbilityDataTable->FindRow<FYogAbilityData>(FName(TEXT("AbilityDataStartUp")), ContextString, true);
	if (AbilityData)
	{
		UE_LOG(LogTemp, Warning, TEXT("Damage: %f, DMGAmplify: %f, MontagePlayRate: %f"), AbilityData->Damage, AbilityData->DMGAmplify, AbilityData->MontagePlayRate);
	}

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

void UYogGameplayAbility::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
	EventOn_AbilityEnded.Broadcast();
}



FYogGameplayEffectContainerSpec UYogGameplayAbility::MakeEffectContainerSpecFromContainer(const FYogGameplayEffectContainer& Container, const FGameplayEventData& EventData, int32 OverrideGameplayLevel)
{
	// First figure out our actor info
	FYogGameplayEffectContainerSpec ReturnSpec;
	AActor* OwningActor = GetOwningActorFromActorInfo();
	AYogCharacterBase* OwningCharacter = Cast<AYogCharacterBase>(OwningActor);
	UYogAbilitySystemComponent* OwningASC = OwningCharacter->GetASC();

	if (OwningASC)
	{
		//@TODO Empty Hit result for now
		if (Container.TargetType.Get())
		{
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
			OverrideGameplayLevel = OverrideGameplayLevel = this->GetAbilityLevel(); //OwningASC->GetDefaultAbilityLevel();
		}

		// Build GameplayEffectSpecs for each applied effect
		for (const TSubclassOf<UGameplayEffect>& EffectClass : Container.TargetGameplayEffectClasses)
		{
			ReturnSpec.TargetGameplayEffectSpecs.Add(MakeOutgoingGameplayEffectSpec(EffectClass, OverrideGameplayLevel));
		}
	}
	return ReturnSpec;
}


FYogGameplayEffectContainerSpec UYogGameplayAbility::MakeEffectContainerSpec(FGameplayTag ContainerTag, const FGameplayEventData& EventData, int32 OverrideGameplayLevel)
{
	AYogCharacterBase* OwningCharacter = GetOwnerCharacterInfo();
	UYogAbilitySystemComponent* ASC = OwningCharacter->GetASC();

	//iterate container map in player 
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
		AllEffects.Append(K2_ApplyGameplayEffectSpecToTarget(SpecHandle, ContainerSpec.TargetData));
	}
	return AllEffects;
}


void UYogGameplayAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	//if (TriggerEventData && bHasBlueprintActivateFromEvent)
	//{
	//	// A Blueprinted ActivateAbility function must call CommitAbility somewhere in its execution chain.
	//	K2_ActivateAbilityFromEvent(*TriggerEventData);
	//}
	//else if (bHasBlueprintActivate)
	//{
	//	// A Blueprinted ActivateAbility function must call CommitAbility somewhere in its execution chain.
	//	K2_ActivateAbility();
	//}
	//else if (bHasBlueprintActivateFromEvent)
	//{
	//	UE_LOG(LogAbilitySystem, Warning, TEXT("Ability %s expects event data but none is being supplied. Use 'Activate Ability' instead of 'Activate Ability From Event' in the Blueprint."), *GetName());
	//	constexpr bool bReplicateEndAbility = false;
	//	constexpr bool bWasCancelled = true;
	//	EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
	//}
	//else
	//{
	//	// Native child classes should override ActivateAbility and call CommitAbility.
	//	// CommitAbility is used to do one last check for spending resources.
	//	// Previous versions of this function called CommitAbility but that prevents the callers
	//	// from knowing the result. Your override should call it and check the result.
	//	// Here is some starter code:

	//	//	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	//	//	{			
	//	//		constexpr bool bReplicateEndAbility = true;
	//	//		constexpr bool bWasCancelled = true;
	//	//		EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
	//	//	}
	//}
}
