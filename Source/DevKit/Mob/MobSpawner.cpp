// Fill out your copyright notice in the Description page of Project Settings.


#include "MobSpawner.h"
#include "DevKit/Controller/YogAIController.h"

// Sets default values
AMobSpawner::AMobSpawner()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

}

// Called when the game starts or when spawned
void AMobSpawner::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AMobSpawner::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

AEnemyCharacterBase* AMobSpawner::SpawnMob(FTransform transform)
{
	//UPROPERTY(EditAnywhere, BlueprintReadWrite)
	//TSubclassOf<AEnemyCharacterBase> EnemyType;

	//UPROPERTY(EditAnywhere, BlueprintReadWrite)
	//TObjectPtr<UBehaviorTree> Behaviour;

	//TSubclassOf<AEnemyCharacterBase> enemy_class = SpawnEnemy->EnemyType;

	//SpawnEnemy->EnemyBT behaviour tree


	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;


	AEnemyCharacterBase* spawned_mob = GetWorld()->SpawnActor<AEnemyCharacterBase>
		(
			SpawnEnemy->EnemyType,
			transform,
			SpawnParams
		);

	return spawned_mob;




	//target_spawner->SetActorRelativeTransform(this->GetActorTransform());



	//if (ReceivingChar->bWeaponEquiped == false)
	//{
	//	//spawn && attach weapon
	//	USkeletalMeshComponent* AttachTarget = ReceivingChar->GetMesh();
	//	for (FWeaponActorToSpawn& WeaponActorInst : WeaponDefinition->ActorsToSpawn)
	//	{
	//		TSubclassOf<AActor> WeaponActorClass = WeaponActorInst.ActorToSpawn;
	//		FName Socket = WeaponActorInst.AttachSocket;
	//		FTransform Transform = WeaponActorInst.AttachTransform;

	//		AActor* WeaponSpawned = GetWorld()->SpawnActorDeferred<AActor>(WeaponActorClass, FTransform::Identity, ReceivingChar);
	//		WeaponSpawned->FinishSpawning(FTransform::Identity, /*bIsDefaultTransform=*/ true);
	//		WeaponSpawned->SetActorRelativeTransform(Transform);
	//		WeaponSpawned->AttachToComponent(AttachTarget, FAttachmentTransformRules::KeepRelativeTransform, Socket);

	//	}
	//	for (const UYogAbilitySet* YogAbilitiesSet : WeaponDefinition->AbilitySetsToGrant)
	//	{
	//		for (FYogAbilitySet_GameplayAbility GameAbilitySet : YogAbilitiesSet->GrantedGameplayAbilities)
	//		{
	//			ReceivingChar->GrantGameplayAbility(GameAbilitySet.Ability, GameAbilitySet.AbilityLevel);
	//		}
	//	}
	//	ReceivingChar->bWeaponEquiped = true;
	//}


}

void AMobSpawner::SetupSpawnMobAI(AEnemyCharacterBase* character)
{
	AYogAIController* controller = Cast<AYogAIController>(character->GetController());
	if (controller)
	{

		
		controller->InitializeAI(SpawnEnemy->EnemyBT);
	}
	

}

