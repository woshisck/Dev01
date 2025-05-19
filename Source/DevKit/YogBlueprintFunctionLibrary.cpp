// Fill out your copyright notice in the Description page of Project Settings.


#include "YogBlueprintFunctionLibrary.h"
#include "Character/YogCharacterBase.h"
#include "Item/Weapon/WeaponDefinition.h"
#include <DevKit/AbilitySystem/YogAbilitySystemComponent.h>

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
			TSubclassOf<AActor> WeaponActorClass = WeaponActorInst.ActorToSpawn;
			FName Socket = WeaponActorInst.AttachSocket;
			FTransform Transform = WeaponActorInst.AttachTransform;

			AActor* NewActor = World->SpawnActorDeferred<AActor>(WeaponActorClass, FTransform::Identity, ReceivingChar);
			NewActor->FinishSpawning(FTransform::Identity, /*bIsDefaultTransform=*/ true);
			NewActor->SetActorRelativeTransform(Transform);
			NewActor->AttachToComponent(AttachTarget, FAttachmentTransformRules::KeepRelativeTransform, Socket);
		}


		for (const UYogAbilitySet* YogAbilitiesSet : WeaponDefinition->AbilitySetsToGrant)
		{
			for (FYogAbilitySet_GameplayAbility GameAbilitySet : YogAbilitiesSet->GrantedGameplayAbilities)
			{
				ReceivingChar->GrantGameplayAbility(GameAbilitySet.Ability, GameAbilitySet.AbilityLevel);
			}
		}
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
