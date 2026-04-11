// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Data/EnemyData.h"
#include "MobSpawner.generated.h"

class AEnemyCharacterBase;

UCLASS()
class DEVKIT_API AMobSpawner : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AMobSpawner();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable)
	AEnemyCharacterBase* SpawnMob(TSubclassOf<AActor> spawn_actor_class);

	/**
	 * 敌人生成后立即调用（在 BP 子类中重写以播放出生特效/动画）
	 * @param SpawnedEnemy   刚刚生成的敌人 Actor
	 * @param SpawnLocation  生成位置（NavMesh 对齐后，可直接在此播放 Niagara）
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Spawning")
	void OnEnemySpawned(AEnemyCharacterBase* SpawnedEnemy, FVector SpawnLocation);

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool SingleSpawn = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawning")
	float SpawnRadius = 1000.f;

	// 生成时在 NavMesh 表面 Z 基础上额外抬高多少（补偿角色 Capsule 半高，防止生成到地下）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawning")
	float SpawnZOffset = 96.f;

	FVector GetRandomReachablePoint();

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<TSubclassOf<AEnemyCharacterBase>> EnemySpawnClassis;



};
