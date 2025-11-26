#include "WeaponDefinition.h"
#include "WeaponInstance.h"
#include "Character/YogCharacterBase.h"


void UWeaponDefinition::SetupWeaponToCharacter(UWorld* World, USkeletalMeshComponent* AttachTarget, AYogCharacterBase* ReceivingChar)
{
	for (FWeaponActorToSpawn& WeaponActorInst : ActorsToSpawn)
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

	//TODO: DEPRECATED : for loop grant ability
	//for (const UYogAbilitySet* YogAbilitiesSet : AbilitySetsToGrant)
	//{
	//	for (FYogAbilitySet_GameplayAbility GameAbilitySet : YogAbilitiesSet->GrantedGameplayAbilities)
	//	{
	//		ReceivingChar->GrantGameplayAbility(GameAbilitySet.Ability, GameAbilitySet.AbilityLevel);
	//	}
	//}

	if (WeaponLayer)
	{
		UAnimInstance* AnimInstance = ReceivingChar->GetMesh()->GetAnimInstance();
		AnimInstance->LinkAnimClassLayers(TSubclassOf<UAnimInstance>(WeaponLayer));
	}
}
