// Fill out your copyright notice in the Description page of Project Settings.


#include "YogGameplayAbility.h"
#include "YogTargetType.h"
#include "../../Character/YogCharacterBase.h"
#include "../YogAbilitySystemComponent.h"
#include "../../Component/HitBoxBufferComponent.h"
#include "Data/AbilityData.h"
#include "AbilitySystemComponent.h"

UYogGameplayAbility::UYogGameplayAbility(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	//NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	



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
	//static const FString ContextString(TEXT("Character Data Lookup"));
	//FActionData* AbilityData = YogAbilityDataTable->FindRow<FActionData>(FName(TEXT("AbilityDataStartUp")), ContextString, true);
	//if (AbilityData)
	//{
	//	UE_LOG(LogTemp, Warning, TEXT("Damage: %f, DMGAmplify: %f, MontagePlayRate: %f"), AbilityData->Damage, AbilityData->DMGAmplify, AbilityData->MontagePlayRate);
	//}

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

void UYogGameplayAbility::SetupAbilityStat(const FActionData& data)
{

	AbilityActData = data;
	ActDamage = data.ActDamage;
	ActRange = data.ActRange;
	ActResilience = data.ActResilience;
	ActDmgReduce = data.ActDmgReduce;
	ActRotateSpeed = data.ActRotateSpeed;
	JumpFrameTime = data.JumpFrameTime;
	FreezeFrameTime = data.FreezeFrameTime;

	MontageToPlay = data.Montage;

	hitbox = data.hitbox;

	//AActor* AvatarActor = GetAvatarActorFromActorInfo();
	//AYogCharacterBase* player = Cast<AYogCharacterBase>(AvatarActor);
	////UYogAbilitySystemComponent* asc = player->GetASC();
	//player->BaseAttributeSet->SetActDamage(data.ActDamage);
	//player->BaseAttributeSet->SetActDamage(data.ActRange);
	//player->BaseAttributeSet->SetActResilience(data.ActResilience);
	//player->BaseAttributeSet->SetActDmgReduce(data.ActDmgReduce);
	//player->BaseAttributeSet->SetActRotateSpeed(data.ActRotateSpeed);
	//player->BaseAttributeSet->SetJumpFrameTime(data.JumpFrameTime);
	//player->BaseAttributeSet->SetFreezeFrameTime(data.FreezeFrameTime);

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
	EventOn_AbilityStart.Broadcast();

	UYogAbilitySystemComponent* ASC = Cast<UYogAbilitySystemComponent>(ActorInfo->AbilitySystemComponent);
	ASC->CurrentAbilitySpecHandle = Handle;

	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	AYogCharacterBase* player = Cast<AYogCharacterBase>(AvatarActor);

	if (!ActionRow.IsNull())
	{
		FActionData* actionData = ActionRow.GetRow<FActionData>(__func__);
		SetupAbilityStat(*actionData);

	}


	//UYogAbilitySystemComponent* asc = player->GetASC();

	//TODO: add loose gameplaytag for blocking ability
	//if (!this->ActivationBlockedTags.IsEmpty())
	//{		
	//	ASC->AddLooseGameplayTags(this->ActivationBlockedTags);
	//}
}


//TODO: NEED TO CHANGE FUNCTION NAME FOR FURTHER DIFFERENT 
FActionData UYogGameplayAbility::GetRowData()
{
	FActionData Result;
	if (!ActionRow.IsNull())
	{
		FActionData* actionData = ActionRow.GetRow<FActionData>(__func__);
		return *actionData;
		//SetupAbilityStat(*actionData);

	}
	return Result;
}

void UYogGameplayAbility::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
	EventOn_AbilityEnded.Broadcast();


	//TODO: remove loose gameplaytag for blocking ability
	//UYogAbilitySystemComponent* ASC = Cast<UYogAbilitySystemComponent>(ActorInfo->AbilitySystemComponent);
	AYogCharacterBase* player = Cast<AYogCharacterBase>(this->GetAvatarActorFromActorInfo());
	/*player->HitboxbuffComponent->Clear();*/
	
	//ASC->RemoveLooseGameplayTags(this->ActivationBlockedTags);


	UYogAbilitySystemComponent* ASC = Cast<UYogAbilitySystemComponent>(ActorInfo->AbilitySystemComponent);
	ASC->CurrentAbilitySpecHandle = Handle;

	player->BaseAttributeSet->ResetAbilityAttribute();
	//player->BaseAttributeSet->SetActDamage(AbilityActData.ActDamage);
	//player->BaseAttributeSet->SetActDamage(AbilityActData.ActRange);
	//player->BaseAttributeSet->SetActResilience(AbilityActData.ActResilience);
	//player->BaseAttributeSet->SetActDmgReduce(AbilityActData.ActDmgReduce);
	//player->BaseAttributeSet->SetActRotateSpeed(AbilityActData.ActRotateSpeed);
	//player->BaseAttributeSet->SetJumpFrameTime(AbilityActData.JumpFrameTime);
	//player->BaseAttributeSet->SetFreezeFrameTime(AbilityActData.FreezeFrameTime);

}
