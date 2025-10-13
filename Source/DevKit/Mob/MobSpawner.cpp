// Fill out your copyright notice in the Description page of Project Settings.


#include "MobSpawner.h"

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

void AMobSpawner::SpawnMob(FTransform transform)
{
	//UPROPERTY(EditAnywhere, BlueprintReadWrite)
	//TSubclassOf<AEnemyCharacterBase> EnemyType;

	//UPROPERTY(EditAnywhere, BlueprintReadWrite)
	//TObjectPtr<UBehaviorTree> Behaviour;




	FVector SpawnLocation = FVector(0.0f, 0.0f, 0.0f);
	FRotator SpawnRotation = FRotator::ZeroRotator;
	FTransform SpawnTransform(SpawnRotation, SpawnLocation);
	//TSubclassOf<AEnemyCharacterBase> enemy_class = SpawnEnemy->EnemyType;

	//SpawnEnemy->EnemyBT behaviour tree


	AEnemyCharacterBase* target_spawner = GetWorld()->SpawnActorDeferred<AEnemyCharacterBase>(SpawnEnemy->EnemyType, transform);

	//ADD SPAWNER COE
	if (target_spawner)
	{
		target_spawner->FinishSpawning(FTransform::Identity, /*bIsDefaultTransform=*/ true);
	}




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

