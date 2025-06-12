// Fill out your copyright notice in the Description page of Project Settings.


#include "YogBlueprintFunctionLibrary.h"
#include "Character/YogCharacterBase.h"
#include "Item/Weapon/WeaponDefinition.h"
#include "Item/Weapon/WeaponInstance.h"
#include <DevKit/AbilitySystem/YogAbilitySystemComponent.h>
#include "Animation/YogAnimInstance.h"
UYogBlueprintFunctionLibrary::UYogBlueprintFunctionLibrary(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

bool UYogBlueprintFunctionLibrary::IsInEditor()
{
	return GIsEditor;
}

FName UYogBlueprintFunctionLibrary::GetDTRow(FString AssetName, int32 rowNum)
{
	FString result;
	result.Append(AssetName);
	result.Append(TEXT("Lvl_"));
	result += FString::Printf(TEXT("%u"), rowNum);

	return FName(result);
}

bool UYogBlueprintFunctionLibrary::GiveWeaponToCharacter(UObject* WorldContextObject, AYogCharacterBase* ReceivingChar, UWeaponDefinition* WeaponDefinition)
{
	if (WorldContextObject)
	{
		UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);

		USkeletalMeshComponent* AttachTarget = ReceivingChar->GetMesh();
		
		//FGameplayTag RunningTag = FGameplayTag::RequestGameplayTag(FName("State.Running"));
		//if (ReceivingChar->GetASC()->HasMatchingGameplayTag(FName("State.Running")))
		//{

		//}
		
		for (FWeaponActorToSpawn& WeaponActorInst : WeaponDefinition->ActorsToSpawn)
		{
			TSubclassOf<AWeaponInstance> WeaponActorClass = WeaponActorInst.ActorToSpawn;
			FName Socket = WeaponActorInst.AttachSocket;
			FTransform Transform = WeaponActorInst.AttachTransform;


			AWeaponInstance* NewActor = World->SpawnActorDeferred<AWeaponInstance>(WeaponActorClass, FTransform::Identity, ReceivingChar);
			//AWeaponInstance* NewActor = World->SpawnActorDeferred<AWeaponInstance>(WeaponActorClass, FTransform::Identity, ReceivingChar);
			NewActor->FinishSpawning(FTransform::Identity, /*bIsDefaultTransform=*/ true);
			NewActor->SetActorRelativeTransform(Transform);
			NewActor->AttachToComponent(AttachTarget, FAttachmentTransformRules::KeepRelativeTransform, Socket);
			ReceivingChar->Weapon = NewActor;

			//AActor* NewActor = World->SpawnActorDeferred<AActor>(WeaponActorClass, FTransform::Identity, ReceivingChar);
			//NewActor->FinishSpawning(FTransform::Identity, /*bIsDefaultTransform=*/ true);
			//NewActor->SetActorRelativeTransform(Transform);
			//NewActor->AttachToComponent(AttachTarget, FAttachmentTransformRules::KeepRelativeTransform, Socket);
		}


		for (const UYogAbilitySet* YogAbilitiesSet : WeaponDefinition->AbilitySetsToGrant)
		{
			for (FYogAbilitySet_GameplayAbility GameAbilitySet : YogAbilitiesSet->GrantedGameplayAbilities)
			{
				ReceivingChar->GrantGameplayAbility(GameAbilitySet.Ability, GameAbilitySet.AbilityLevel);
			}
		}
		if (WeaponDefinition->WeaponLayer)
		{
			UAnimInstance* AnimInstance = ReceivingChar->GetMesh()->GetAnimInstance();
			AnimInstance->LinkAnimClassLayers(TSubclassOf<UAnimInstance>(WeaponDefinition->WeaponLayer));
		}
		
	/*	LinkAnimClassLayers(WeaponDefinition->WeaponLayer);*/

		return true;
	}
	else
	{
		return false;
	}

}

bool UYogBlueprintFunctionLibrary::GiveAbilityToCharacter(UObject* WorldContextObject, AYogCharacterBase* ReceivingChar, UYogAbilitySet* AbilitySet)
{
	if (WorldContextObject)
	{
		UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);

		//TArray<FYogAbilitySet_GameplayAbility> GrantedGameplayAbilities;

		for (FYogAbilitySet_GameplayAbility GameAbilitySet : AbilitySet->GrantedGameplayAbilities)
		{
			ReceivingChar->GrantGameplayAbility(GameAbilitySet.Ability, GameAbilitySet.AbilityLevel);
		}
		
		return true;
	}
	else
	{
		return false;
	}
}
bool UYogBlueprintFunctionLibrary::GiveEffectToCharacter(UObject* WorldContextObject, AYogCharacterBase* ReceivingChar, UWeaponDefinition* WeaponDefinition)
{
	return false;
}

UAnimSequence* UYogBlueprintFunctionLibrary::GetCurrentSlotAnimation(USkeletalMeshComponent* MeshComp, FName SlotName)
{
	if (!MeshComp || !MeshComp->GetAnimInstance()) return nullptr;

	UAnimInstance* AnimInstance = MeshComp->GetAnimInstance();
	FAnimMontageInstance* MontageInstance = AnimInstance->GetActiveMontageInstance();

	if (!MontageInstance || !MontageInstance->Montage) return nullptr;

	// Get the current position in the montage
	float CurrentPosition = MontageInstance->GetPosition();

	// Find the slot track
	const FSlotAnimationTrack* SlotTrack = MontageInstance->Montage->SlotAnimTracks.FindByPredicate(
		[SlotName](const FSlotAnimationTrack& Track) { return Track.SlotName == SlotName; });

	if (SlotTrack)
	{
		// Find which segment is currently playing
		const FAnimSegment* CurrentSegment = SlotTrack->AnimTrack.GetSegmentAtTime(CurrentPosition);
		if (CurrentSegment && CurrentSegment->GetAnimReference())
		{
			return Cast<UAnimSequence>(CurrentSegment->GetAnimReference());
		}
	}

	return nullptr;
}

UAnimSequence* UYogBlueprintFunctionLibrary::SetSequenceRootMotion(UAnimSequence* sequence, bool enableRoot)
{
	sequence->bRootMotionSettingsCopiedFromMontage = false;
	sequence->EnableRootMotionSettingFromMontage(enableRoot ,  ERootMotionRootLock::RefPose);
	return sequence;
}

