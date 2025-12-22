// Fill out your copyright notice in the Description page of Project Settings.


#include "MobSpawner.h"
#include "DevKit/Controller/YogAIController.h"
#include "DevKit/Enemy/EnemyCharacterBase.h"


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

void AMobSpawner::SpawnMob(TSubclassOf<AEnemyCharacterBase> mob_spawn)
{
}
