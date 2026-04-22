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

	// 仅计算随机可达位置，不 SpawnActor（供延迟刷出路径使用）
	UFUNCTION(BlueprintCallable)
	FVector PrepareSpawnLocation();

	// 在指定位置刷出敌人（跳过位置随机化）
	UFUNCTION(BlueprintCallable)
	AEnemyCharacterBase* SpawnMobAtLocation(TSubclassOf<AActor> EnemyClass, FVector Location);

	/**
	 * 敌人生成后立即调用（在 BP 子类中重写以播放出生特效/动画）
	 * @param SpawnedEnemy   刚刚生成的敌人 Actor
	 * @param SpawnLocation  生成位置（NavMesh 对齐后，可直接在此播放 Niagara）
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Spawning")
	void OnEnemySpawned(AEnemyCharacterBase* SpawnedEnemy, FVector SpawnLocation);

	// 预生成 FX 蓝图事件（可选扩展，C++ 已自动播 Niagara；此事件供 BP 叠加额外效果）
	UFUNCTION(BlueprintImplementableEvent, Category = "Spawning")
	void OnPreSpawnFX(FVector SpawnLocation, float FXDuration);

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool SingleSpawn = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawning")
	float SpawnRadius = 1000.f;

	// 生成时在 NavMesh 表面 Z 基础上额外抬高多少（补偿角色 Capsule 半高，防止生成到地下）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawning")
	float SpawnZOffset = 96.f;

	// 预生成 FX 时长随机浮动幅度（±秒），各 Spawner 可独立配置
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawning")
	float SpawnFXVariance = 0.1f;

	FVector GetRandomReachablePoint();

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<TSubclassOf<AEnemyCharacterBase>> EnemySpawnClassis;



};
