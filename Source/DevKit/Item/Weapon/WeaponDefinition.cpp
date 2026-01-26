#include "WeaponDefinition.h"
#include "WeaponInstance.h"
#include "Player/PlayerCharacterBase.h"


void UWeaponDefinition::SetupWeaponToCharacter(USkeletalMeshComponent* AttachTarget, APlayerCharacterBase* ReceivingChar)
{
	//ASAP: TODO: save the param to weapon actor(WeaponInstance)?
	for (FWeaponSpawnData& WeaponActorInst : ActorsToSpawn)
	{
		TSubclassOf<AWeaponInstance> WeaponActorClass = WeaponActorInst.ActorToSpawn;
		FName Socket = WeaponActorInst.AttachSocket;
		FTransform Transform = WeaponActorInst.AttachTransform;


		
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		AWeaponInstance* NewActor = ReceivingChar->GetWorld()->SpawnActor<AWeaponInstance>(WeaponActorClass, FTransform::Identity, SpawnParams);
		//AWeaponInstance* NewActor = World->SpawnActorDeferred<AWeaponInstance>(WeaponActorClass, FTransform::Identity, ReceivingChar);
		
		NewActor->AttachSocket = Socket;
		NewActor->AttachTransform = Transform;
		NewActor->WeaponLayer = WeaponLayer;

		NewActor->EquipWeaponToCharacter(ReceivingChar);
		
		//NewActor->SetActorRelativeTransform(Transform);
		//NewActor->AttachToComponent(AttachTarget, FAttachmentTransformRules::KeepRelativeTransform, Socket);
		//NewActor->FinishSpawning(FTransform::Identity, /*bIsDefaultTransform=*/ true);
		

		ReceivingChar->AbilityData = AbilityData;
	}

	//TODO: DEPRECATED : for loop grant ability
	//for (const UYogAbilitySet* YogAbilitiesSet : AbilitySetsToGrant)
	//{
	//	for (FYogAbilitySet_GameplayAbility GameAbilitySet : YogAbilitiesSet->GrantedGameplayAbilities)
	//	{
	//		ReceivingChar->GrantGameplayAbility(GameAbilitySet.Ability, GameAbilitySet.AbilityLevel);
	//	}
	//}

	//if (WeaponLayer)
	//{
	//	UAnimInstance* AnimInstance = ReceivingChar->GetMesh()->GetAnimInstance();
	//	AnimInstance->LinkAnimClassLayers(TSubclassOf<UAnimInstance>(WeaponLayer));
	//}
}
