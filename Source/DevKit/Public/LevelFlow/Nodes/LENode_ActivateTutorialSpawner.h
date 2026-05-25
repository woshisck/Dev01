#pragma once

#include "CoreMinimal.h"
#include "LevelFlow/Nodes/LENode_Base.h"
#include "LENode_ActivateTutorialSpawner.generated.h"

/**
 * 按 Actor Tag 找到关卡中的 ATutorialMobSpawner 并调用 Activate()。
 * 通常由武器拾取的 NodeEventFlow FA 触发。
 */
UCLASS(meta = (DisplayName = "Activate Tutorial Mob Spawner"))
class DEVKIT_API ULENode_ActivateTutorialSpawner : public ULENode_Base
{
	GENERATED_UCLASS_BODY()

public:
	/** 与 ATutorialMobSpawner 上设置的 Actor Tag 匹配。 */
	UPROPERTY(EditAnywhere, Category = "Spawner")
	FName SpawnerActorTag;

protected:
	virtual void ExecuteInput(const FName& PinName) override;
};
