#include "Item/Weapon/WeaponDefinition.h"
#include "Item/Weapon/WeaponInstance.h"
#include "Character/PlayerCharacterBase.h"
#include "Component/CharacterDataComponent.h"
#include "Engine/AssetManager.h"

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
		NewActor->WeaponLayer = WeaponActorInst.WeaponLayer;

		NewActor->EquipWeaponToCharacter(ReceivingChar);
		
		//NewActor->SetActorRelativeTransform(Transform);
		//NewActor->AttachToComponent(AttachTarget, FAttachmentTransformRules::KeepRelativeTransform, Socket);
		//NewActor->FinishSpawning(FTransform::Identity, /*bIsDefaultTransform=*/ true);
		

		{
			UCharacterData* CD = ReceivingChar->GetCharacterDataComponent()->GetCharacterData();
			UE_LOG(LogTemp, Warning, TEXT("[WeaponSetup][WeaponDefinition] Owner=%s | CD=%s IsCDO=%d IsTransient=%d | NewAbilityData=%s"),
				*ReceivingChar->GetName(),
				CD ? *CD->GetName() : TEXT("null"),
				CD ? (int32)CD->HasAnyFlags(RF_ClassDefaultObject) : -1,
				CD ? (int32)CD->HasAnyFlags(RF_Transient) : -1,
				WeaponAbilityData ? *WeaponAbilityData->GetName() : TEXT("null"));
			CD->AbilityData = WeaponAbilityData;
		}
	}

	// 记录当前装备的武器 DA，供切关时写入 RunState
	ReceivingChar->EquippedWeaponDef = this;

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
