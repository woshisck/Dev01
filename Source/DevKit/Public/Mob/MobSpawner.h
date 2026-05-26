// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Data/EnemyData.h"
#include "MobSpawner.generated.h"

class AEnemyCharacterBase;
class AYogCharacterBase;
class UStoryEncounterPointDA;
struct FBuffFlowLifecycleContext;

USTRUCT(BlueprintType)
struct DEVKIT_API FStoryMobSpawnOptions
{
	GENERATED_BODY()

	/** 为空时使用 Spawner 的 EnemySpawnClassis[0]。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story")
	TSubclassOf<AEnemyCharacterBase> EnemyClassOverride;

	/** true 时在 Spawner 位置生成；false 时走普通 PrepareSpawnLocation 随机点。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story")
	bool bSpawnAtSpawnerLocation = true;

	/** Story 生成物是否计入房间清怪/击杀进度。教程木头人应为 false。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story")
	bool bCountsForLevelClear = false;

	/** 生成后是否从 GameMode 敌人感知列表移除。教程/展示敌人通常为 true。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story")
	bool bUnregisterFromEnemyAwareness = true;

	/** 大于 0 时覆盖 MaxHealth/Health；0 表示使用敌人自身数据。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story", meta = (ClampMin = "0.0"))
	float MaxHealthOverride = 0.0f;

	/** 旧的 Story 生成物仍存活时，再次触发是否先销毁旧实例。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story")
	bool bDestroyExistingStoryMob = true;

	/** Story 生成物死亡时触发的剧情节点。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story")
	TObjectPtr<UStoryEncounterPointDA> OnKillEncounterPoint;

	/** Story 生成物死亡后是否自动重刷。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story")
	bool bRespawnOnDeath = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Story", meta = (ClampMin = "0.0"))
	float RespawnDelay = 5.0f;
};

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
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

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

	/** Story/教程使用的通用控制生成接口。仍复用普通 MobSpawner 的生成 FX、AI Controller、生成点逻辑。 */
	UFUNCTION(BlueprintCallable, Category = "Story")
	AEnemyCharacterBase* SpawnMobForStory(const FStoryMobSpawnOptions& Options);

	void HandleLifecycleStoryEnemySpawned(AEnemyCharacterBase* SpawnedEnemy, FBuffFlowLifecycleContext& Context);
	void HandleLifecycleStoryEnemySpawnFailed(FBuffFlowLifecycleContext& Context);

	UFUNCTION(BlueprintCallable, Category = "Story")
	void ClearStorySpawnedMob();

	/**
	 * 敌人生成后立即调用（在 BP 子类中重写以播放出生特效/动画）
	 * @param SpawnedEnemy   刚刚生成的敌人 Actor
	 * @param SpawnLocation  生成位置（NavMesh 对齐后，可直接在此播放 Niagara）
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Spawning")
	void OnEnemySpawned(AEnemyCharacterBase* SpawnedEnemy, FVector SpawnLocation);

	// 预生成 FX 蓝图事件（可选扩展，C++ 已自动播 Niagara；此事件供 BP 叠加额外效果）
	UFUNCTION(BlueprintImplementableEvent, Category = "Spawning")
	void OnPreSpawnFX(FVector SpawnLocation, float FXDuration, float FXScale);

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

	// 预生成 FX 整体缩放（传给 Niagara User.Scale 变量）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawning")
	float SpawnFXScale = 1.0f;

	FVector GetRandomReachablePoint();

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<TSubclassOf<AEnemyCharacterBase>> EnemySpawnClassis;


private:
	void ConfigureStorySpawnedMob(AEnemyCharacterBase* Mob, const FStoryMobSpawnOptions& Options) const;
	void ApplyStoryHealthOverride(AEnemyCharacterBase* Mob, const FStoryMobSpawnOptions& Options) const;
	void HandleStoryMobDeathStarted(AYogCharacterBase* Mob);
	void HandleStoryMobDied(AYogCharacterBase* Mob);
	void TriggerStoryKillEncounter(AYogCharacterBase* Mob);
	void RespawnStoryMob();

	UPROPERTY()
	TWeakObjectPtr<AEnemyCharacterBase> StorySpawnedMob;

	UPROPERTY()
	FStoryMobSpawnOptions ActiveStorySpawnOptions;
	FTimerHandle StoryRespawnTimer;
	FDelegateHandle StoryDeathStartedDelegateHandle;
	FDelegateHandle StoryDeathDelegateHandle;
	bool bStoryKillEncounterTriggered = false;

};
