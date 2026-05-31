#pragma once

#include "CoreMinimal.h"
#include "Story/Flow/Nodes/SNode_Base.h"
#include "SNode_ActivateTutorialSpawner.generated.h"

class AEnemyCharacterBase;
class UStoryEncounterPointDA;

/**
 * 按 Actor Tag 找到场景中的 AMobSpawner，并用普通 Spawner 的 Story Spawn 接口生成敌人。
 * 类名保留为 ActivateTutorialSpawner 以兼容已有 FA 资产。
 */
UCLASS(meta = (DisplayName = "Spawn Mob From Spawner"))
class DEVKIT_API USNode_ActivateTutorialSpawner : public USNode_Base
{
	GENERATED_UCLASS_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Spawner")
	FName SpawnerActorTag;

	UPROPERTY(EditAnywhere, Category = "Spawner")
	TSubclassOf<AEnemyCharacterBase> EnemyClassOverride;

	UPROPERTY(EditAnywhere, Category = "Spawner")
	bool bSpawnAtSpawnerLocation = true;

	UPROPERTY(EditAnywhere, Category = "Spawner")
	bool bCountsForLevelClear = false;

	UPROPERTY(EditAnywhere, Category = "Spawner")
	bool bUnregisterFromEnemyAwareness = true;

	UPROPERTY(EditAnywhere, Category = "Spawner", meta = (ClampMin = "0.0"))
	float MaxHealthOverride = 0.0f;

	UPROPERTY(EditAnywhere, Category = "Respawn")
	bool bRespawnOnDeath = true;

	UPROPERTY(EditAnywhere, Category = "Respawn", meta = (ClampMin = "0.0"))
	float RespawnDelay = 2.0f;

	UPROPERTY(EditAnywhere, Category = "Story")
	TObjectPtr<UStoryEncounterPointDA> OnKillEncounterPoint;

protected:
	virtual void ExecuteInput(const FName& PinName) override;
};
