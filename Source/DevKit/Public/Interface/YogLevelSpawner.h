// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Templates/SubclassOf.h"
#include "UObject/Interface.h"
#include "YogLevelSpawner.generated.h"

class AEnemyCharacterBase;
class UEnemyData;
class UEnemyWeaponDefinition;
class UItemDefinition;
class UWeaponDefinition;

UENUM(BlueprintType)
enum class EYogSpawnItemType : uint8
{
	Pickup	UMETA(DisplayName = "Pickup"),
	Weapon	UMETA(DisplayName = "Weapon"),
};

USTRUCT(BlueprintType)
struct DEVKIT_API FYogEnemySpawnRequest
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level Spawn|Enemy")
	TObjectPtr<UEnemyData> EnemyData = nullptr;

	/** Wins over the Actor class carried by EnemyData when set. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level Spawn|Enemy")
	TSubclassOf<AEnemyCharacterBase> EnemyClassOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level Spawn|Enemy")
	TObjectPtr<UEnemyWeaponDefinition> WeaponDefinitionOverride = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level Spawn|Enemy")
	FTransform SpawnTransform = FTransform::Identity;

	/** Above zero, SpawnTransform is a search origin and the implementer picks a reachable point inside the radius. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level Spawn|Enemy", meta = (ClampMin = "0.0"))
	float ScatterRadius = 0.f;

	/** False keeps the enemy out of the room clear and kill counters, as tutorial and showcase enemies need. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level Spawn|Enemy")
	bool bCountsForLevelClear = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level Spawn|Enemy")
	bool bApplyRoomBuffs = true;
};

USTRUCT(BlueprintType)
struct DEVKIT_API FYogItemSpawnRequest
{
	GENERATED_BODY()

	/** Selects which definition field below is read. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level Spawn|Item")
	EYogSpawnItemType ItemType = EYogSpawnItemType::Pickup;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level Spawn|Item",
		meta = (EditCondition = "ItemType == EYogSpawnItemType::Pickup", EditConditionHides))
	TObjectPtr<UItemDefinition> ItemDefinition = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level Spawn|Item",
		meta = (EditCondition = "ItemType == EYogSpawnItemType::Weapon", EditConditionHides))
	TObjectPtr<UWeaponDefinition> WeaponDefinition = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level Spawn|Item")
	FTransform SpawnTransform = FTransform::Identity;

	/** Replaces the implementer's default spawner Actor class for this one request. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level Spawn|Item")
	TSubclassOf<AActor> SpawnerClassOverride;
};

UINTERFACE(MinimalAPI, Blueprintable)
class UYogLevelSpawner : public UInterface
{
	GENERATED_BODY()
};

/**
 * Level-scoped spawn service: lets gameplay code ask "the level" for an enemy or an item without
 * knowing whether a MobSpawner, the GameMode or a hand-placed Actor ends up doing the work.
 *
 * Both functions are BlueprintImplementableEvent, so implementers must be Blueprints — a Level
 * Blueprint deriving from AYogLevelScript, or a Blueprint subclass of AYogGameMode. Native classes
 * cannot supply a body. Call through the generated thunks rather than casting:
 *
 *     if (Target->Implements<UYogLevelSpawner>())
 *     {
 *         IYogLevelSpawner::Execute_SpawnEnemy(Target, Request);
 *     }
 */
class DEVKIT_API IYogLevelSpawner
{
	GENERATED_BODY()

public:
	/** Returns the spawned enemy, or null when the request could not be placed. */
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "Level Spawn")
	AEnemyCharacterBase* SpawnEnemy(const FYogEnemySpawnRequest& Request);

	/** Returns the spawned pickup or weapon spawner Actor, or null when the request could not be placed. */
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "Level Spawn")
	AActor* SpawnItem(const FYogItemSpawnRequest& Request);
};
