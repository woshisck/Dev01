#pragma once

#include "CoreMinimal.h"
#include "Story/Flow/Nodes/SNode_Base.h"
#include "SNode_ActivateTutorialSpawner.generated.h"

/**
 * 按 Actor Tag 找到场景中的 ATutorialMobSpawner 并调用 Activate()。
 * Story FA 版本，功能与 LENode_ActivateTutorialSpawner 相同。
 */
UCLASS(meta = (DisplayName = "Activate Tutorial Mob Spawner"))
class DEVKIT_API USNode_ActivateTutorialSpawner : public USNode_Base
{
	GENERATED_UCLASS_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Spawner")
	FName SpawnerActorTag;

protected:
	virtual void ExecuteInput(const FName& PinName) override;
};
