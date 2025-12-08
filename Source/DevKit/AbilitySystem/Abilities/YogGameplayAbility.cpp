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



TArray<FActiveGameplayEffectHandle> UYogGameplayAbility::ApplyEffectContainer(FGameplayTag ContainerTag, const FGameplayEventData& EventData, int32 OverrideGameplayLevel )
{
	FYogGameplayEffectContainerSpec Spec = MakeEffectContainerSpec(ContainerTag, EventData, OverrideGameplayLevel);
	return ApplyEffectContainerSpec(Spec);
}


void UYogGameplayAbility::UpdateRetrigger(bool retriggerable)
{
	this->bRetriggerInstancedAbility = retriggerable;

}

FActionData UYogGameplayAbility::GetActionData()
{
	return AbilityActData;
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

void UYogGameplayAbility::SetupActionData(FActionData& action_data)
{

	ActDamage = action_data.ActDamage;
	ActRange = action_data.ActRange;
	ActResilience = action_data.ActResilience;
	ActRotateSpeed = action_data.ActRotateSpeed;
	JumpFrameTime = action_data.JumpFrameTime;
	FreezeFrameTime = action_data.FreezeFrameTime;
	Montage = action_data.Montage;

	hitboxTypes = action_data.hitboxTypes;


	//switch (action_data.hitboxType)
	//{
	//case EHitBoxType::Circle:
	//	HitboxCircles = action_data.HitboxCircles;
	//	break;
	//case EHitBoxType::Annulus:
	//	AnnulusHitbox = action_data.AnnulusHitbox;
	//	break;
	//case EHitBoxType::Square:
	//	HitboxSquares = action_data.HitboxSquares;
	//	break;
	//case EHitBoxType::Triangle:
	//	HitboxTriangles = action_data.HitboxTriangles;
	//	break;

	//default:
	//	break;
	//}

	//hitbox.SetNumUninitialized(action_data.hitbox.Num());
	//FMemory::Memcpy(hitbox.GetData(), action_data.hitbox.GetData(), action_data.hitbox.Num() * sizeof(FHitBoxData));
	//for (auto& point : hitbox)
	//{
	//	UE_LOG(LogTemp, Warning, TEXT("this->box Point Vector: %s"), *point.Point.ToString());
	//}
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
			OverrideGameplayLevel = this->GetAbilityLevel(); //OwningASC->GetDefaultAbilityLevel();
		}

		// Build GameplayEffectSpecs for each applied effect
		//for (const TSubclassOf<UGameplayEffect>& EffectClass : Container.EffectClasses)
		for (const FYogEffectPorperty& YogEffectProperty : Container.EffectClasses)
		{
			ReturnSpec.TargetGameplayEffectSpecs.Add(MakeOutgoingGameplayEffectSpec(YogEffectProperty.GameplayEffect, YogEffectProperty.EffectLevel));
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

	player->UpdateCharacterState(EYogCharacterState::OnAction, FVector(0,0,0));

	//for (FVector& point : array_hitbox_vector)
	//{
	//	UE_LOG(LogTemp, Warning, TEXT("hitbox_point_array: %s"), *point.ToString());
	//}

	//for (auto& point : hitbox)
	//{
	//	UE_LOG(LogTemp, Warning, TEXT("this->box Point Vector: %s"), *point.Point.ToString());
	//}

	player->AttributeStatsComponent->AddAttribute(player->BaseAttributeSet->GetAttackAttribute(), ActDamage);
	player->AttributeStatsComponent->AddAttribute(player->BaseAttributeSet->GetAttackRangeAttribute(), ActRange);
	player->AttributeStatsComponent->AddAttribute(player->BaseAttributeSet->GetResilienceAttribute(), ActResilience);
	player->AttributeStatsComponent->AddAttribute(player->BaseAttributeSet->GetDmgTakenAttribute(), ActDmgReduce);

	//UYogAbilitySystemComponent* asc = player->GetASC();

	//TODO: add loose gameplaytag for blocking ability
	//if (!this->ActivationBlockedTags.IsEmpty())
	//{		
	//	ASC->AddLooseGameplayTags(this->ActivationBlockedTags);
	//}
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


	//TODO: remove loose gameplaytag for blocking ability
	//UYogAbilitySystemComponent* ASC = Cast<UYogAbilitySystemComponent>(ActorInfo->AbilitySystemComponent);
	AYogCharacterBase* player = Cast<AYogCharacterBase>(this->GetAvatarActorFromActorInfo());
	/*player->HitboxbuffComponent->Clear();*/
	
	//ASC->RemoveLooseGameplayTags(this->ActivationBlockedTags);


	UYogAbilitySystemComponent* ASC = Cast<UYogAbilitySystemComponent>(ActorInfo->AbilitySystemComponent);
	ASC->CurrentAbilitySpecHandle = Handle;

	player->UpdateCharacterState(EYogCharacterState::Idle, FVector(0, 0, 0));


	player->AttributeStatsComponent->AddAttribute(player->BaseAttributeSet->GetAttackAttribute(), -ActDamage);
	player->AttributeStatsComponent->AddAttribute(player->BaseAttributeSet->GetAttackRangeAttribute(), -ActRange);
	player->AttributeStatsComponent->AddAttribute(player->BaseAttributeSet->GetResilienceAttribute(), -ActResilience);
	player->AttributeStatsComponent->AddAttribute(player->BaseAttributeSet->GetDmgTakenAttribute(), -ActDmgReduce);


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
