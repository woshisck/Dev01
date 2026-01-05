// Fill out your copyright notice in the Description page of Project Settings.


#include "YogLevelScript.h"
#include <DevKit/Map/YogMapDefinition.h>
#include "Portal.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/AssetManager.h"
#include "DevAssetManager.h"
#include "DevKit/GameModes/YogGameMode.h"
#include "DevKit/AbilitySystem/YogAbilitySystemComponent.h"

#include "DevKit/Mob/MobSpawner.h"

void AYogLevelScript::PreInitializeComponents()
{
	UE_LOG(LogTemp, Warning, TEXT("Start pre initialize component"));
	
	FWorldDelegates::OnPostWorldCreation.AddUObject(this, &AYogLevelScript::OnLevelLoaded);

	
}

void AYogLevelScript::LevelSetup()
{
	if (!Mapdefinition)
	{
		return;
	}

	TArray<AActor*> OutActors;
	TArray<AEnemyCharacterBase*> Enemies;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AEnemyCharacterBase::StaticClass(), OutActors);

	if (OutActors.Num() <= 0)
	{
		return;
	}

	for (AActor* a : OutActors)
	{
		Enemies.Add(Cast<AEnemyCharacterBase>(a));
	}

	for (const FMapFeature feature : Mapdefinition->MapFeatures)
	{

		for (AEnemyCharacterBase* enemy : Enemies)
		{

			/*FGameplayAbilitySpecHandle K2_GiveAbility(TSubclassOf<UGameplayAbility> AbilityClass, int32 Level = 0, int32 InputID = -1);*/
			enemy->GetASC()->K2_GiveAbility(feature.GainPassiveAbility, feature.level, -1);
				
		}

	}
}

void AYogLevelScript::OnLevelLoaded(UWorld* LoadedWorld)
{
	LevelStart.Broadcast();
	/*LevelSetup(this->Mapdefinition);*/
}

void AYogLevelScript::BeginPlay()
{
	Super::BeginPlay();

	if (Mapdefinition)
	{


		//TArray<AActor*> OutActors;
		//TArray<AMobSpawner*> MobSpawners;
		//UGameplayStatics::GetAllActorsOfClass(GetWorld(), AMobSpawner::StaticClass(), OutActors);


		//if (OutActors.Num() <= 0)
		//{
		//	return;
		//}

		//for (AActor* a : OutActors)
		//{
		//	MobSpawners.Add(Cast<AMobSpawner>(a));
		//}


		//switch (Mapdefinition->EnemySpawnRule)
		//{
		//case EEnemySpawnRule::OneByOne:
		//	for (int i = 0; i < Mapdefinition->TotalCount; i++)
		//	{
		//		int32 RandomIndex = FMath::RandHelper(MobSpawners.Num());
		//		AMobSpawner* Spawner = MobSpawners[RandomIndex];
		//		Spawner->SpawnMob();
		//	}
		//	break;
		//case EEnemySpawnRule::AllInOnce:
		//	break;
		//case EEnemySpawnRule::Wave:
		//	break;
		//case EEnemySpawnRule::Default:
		//	break;
		//default:
		//	break;
		//}

	}
	//TODO: Make it Deprecated for future Setting


	//TArray<AActor*> FoundActors;
	//UGameplayStatics::GetAllActorsOfClass(GetWorld(), APortal::StaticClass(), FoundActors);
	//if (FoundActors.Num() > 0)
	//{
	//	for (AActor* portalActor : FoundActors)
	//	{
	//		APortal* portal = Cast<APortal>(portalActor);
	//		for (const FPortalEntry& level_portal : Mapdefinition->LevelPortals)
	//		{
	//			
	//			int gate_index = level_portal.GateIndex;
	//			
	//			if (gate_index == portal->Index)
	//			{
	//				portal->NextLevels = level_portal.NextLevels;
	//			}
	//		}
	//		//for(const int& portal_index : this->Mapdefinition.levelportals)
	//		//int portal_index = this->Mapdefinition->LevelPortals
	//	}
	//}
	//for (const auto& level_portal_data : Mapdefinition->LevelPortals)
	//{
	//	for (const auto& next_level_data : level_portal_data.NextLevels)
	//	{
	//		UDevAssetManager* AssetManager = UDevAssetManager::GetDevAssetManager();
	//		FSoftObjectPath path = next_level_data.LevelMapSoftPtr.ToSoftObjectPath();
	//		AssetManager->AsyncLoadAsset(path, AssetManager->OnLoadFinished);
	//	}
	//}
	//AYogGameMode* GM = Cast<AYogGameMode>(UGameplayStatics::GetGameMode(this));
	//GM->MonsterKillCount = 0;
}