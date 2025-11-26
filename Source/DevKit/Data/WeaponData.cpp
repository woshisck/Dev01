// Fill out your copyright notice in the Description page of Project Settings.


#include "WeaponData.h"
#include "Character/YogCharacterBase.h"
#include "AbilitySystem/Abilities/YogGameplayAbility.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "Data/AbilityData.h"
#include "DevKit/Item/Weapon/WeaponInstance.h"

const FWeaponAttributeData& UWeaponData::GetWeaponData() const
{
	if (!WeaponAttributeRow.IsNull())
	{
		FWeaponAttributeData* wpnData = WeaponAttributeRow.GetRow<FWeaponAttributeData>(__func__);
		if (wpnData)
		{
			return *wpnData;
		}
	}

	return DefaultWPNData;
	// TODO: insert return statement here
}


void UWeaponData::GrantAbilityToOwner(AYogCharacterBase* Owner)
{
	



	/*TArray<TObjectPtr<UAbilityData>> Action;*/
	//UYogAbilitySystemComponent* asc = Owner->GetASC();
	//if (Action.Num() > 0)
	//{
	//	for (const UAbilityData* ability_action : Action)
	//	{
	//		//TSubclassOf<UYogGameplayAbility> ability;
	//		//ability_action->ability
	//		for (const TSubclassOf<UYogGameplayAbility> ability_class : ability_action->abilities)
	//		{
	//			FGameplayAbilitySpec abilitySpec(ability_class, 0);
	//			asc->GiveAbility(abilitySpec);
	//		}
	//	}
	//}


}

void UWeaponData::GiveWeapon(UWorld* World, USkeletalMeshComponent* AttachTarget, AYogCharacterBase* ReceivingChar)
{
	for (FWeaponHoldData& WeaponActorInst : ActorsToSpawn)
	{
		TSubclassOf<AWeaponInstance> WeaponActorClass = WeaponActorInst.ActorToSpawn;
		FName Socket = WeaponActorInst.AttachSocket;
		FTransform Transform = WeaponActorInst.AttachTransform;


		AWeaponInstance* NewActor = World->SpawnActorDeferred<AWeaponInstance>(WeaponActorClass, FTransform::Identity, ReceivingChar);
		//AWeaponInstance* NewActor = World->SpawnActorDeferred<AWeaponInstance>(WeaponActorClass, FTransform::Identity, ReceivingChar);

		NewActor->SetActorRelativeTransform(Transform);
		NewActor->AttachToComponent(AttachTarget, FAttachmentTransformRules::KeepRelativeTransform, Socket);
		NewActor->FinishSpawning(FTransform::Identity, /*bIsDefaultTransform=*/ true);
		

	}

	if (WeaponLayer)
	{
		UAnimInstance* AnimInstance = ReceivingChar->GetMesh()->GetAnimInstance();
		AnimInstance->LinkAnimClassLayers(TSubclassOf<UAnimInstance>(WeaponLayer));
	}
}
