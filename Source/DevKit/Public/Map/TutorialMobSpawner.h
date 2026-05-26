#pragma once

#include "CoreMinimal.h"
#include "Mob/MobSpawner.h"
#include "TutorialMobSpawner.generated.h"

class AEnemyCharacterBase;
class AYogCharacterBase;
class UStoryEncounterPointDA;

/**
 * AMobSpawner 的教程子类。
 * 调用 Activate() 后手动刷出一只敌人（复用父类生成 FX 和 Controller 逻辑）；
 * 生成的敌人不参与房间清怪判定，死亡后等待 RespawnDelay 秒自动重新刷出。
 * 可选绑定 OnKillEncounterPoint — 首次击杀时触发剧情节点（受节点 FirePolicy 控制）。
 */
UCLASS(Blueprintable)
class DEVKIT_API ATutorialMobSpawner : public AMobSpawner
{
	GENERATED_BODY()

public:
	ATutorialMobSpawner();

	/** 击杀后等待多少秒再重新生成（默认 5s）。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial|Spawn", meta = (ClampMin = "0.0"))
	float RespawnDelay = 5.0f;

	/** 大于 0 时覆盖教程敌人的 MaxHealth/Health，避免训练木头人被默认低血量一刀击杀。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial|Spawn", meta = (ClampMin = "0.0"))
	float SpawnedMobMaxHealthOverride = 100.0f;

	/** 为 true 时在 BeginPlay 直接激活，用于调试。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial|Spawn")
	bool bActivateOnBeginPlay = false;

	/** Mob 死亡时触发的剧情节点，受节点 FirePolicy 控制（通常仅触发一次）。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tutorial|Story")
	TObjectPtr<UStoryEncounterPointDA> OnKillEncounterPoint;

	/** 由武器拾取 FA 触发；已激活则无效。生成的 Mob 不计入房间清怪。 */
	UFUNCTION(BlueprintCallable, Category = "Tutorial")
	void Activate();

	UFUNCTION(BlueprintPure, Category = "Tutorial")
	bool IsActive() const { return bIsActive; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

private:
	AEnemyCharacterBase* SpawnTutorialMobAtLocation(TSubclassOf<AEnemyCharacterBase> EnemyClass, const FVector& SpawnLocation);
	void ConfigureSpawnedMob(AEnemyCharacterBase* Mob) const;
	void ApplySpawnedMobHealthOverride(AEnemyCharacterBase* Mob) const;
	void DoSpawnMob();
	void HandleMobDied(AYogCharacterBase* Mob);

	UPROPERTY()
	TWeakObjectPtr<AEnemyCharacterBase> SpawnedMob;

	FTimerHandle RespawnTimer;
	FDelegateHandle DeathDelegateHandle;
	bool bIsActive = false;
};
