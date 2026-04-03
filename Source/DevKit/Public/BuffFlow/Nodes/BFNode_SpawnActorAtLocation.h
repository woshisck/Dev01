#pragma once

#include "CoreMinimal.h"
#include "BuffFlow/Nodes/BFNode_Base.h"
#include "BFNode_SpawnActorAtLocation.generated.h"

/**
 * 即时节点：在指定位置生成 Actor
 * 默认使用 BFC->LastKillLocation（由 BFNode_OnKill 写入）
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "在位置生成Actor", Category = "BuffFlow|动作"))
class DEVKIT_API UBFNode_SpawnActorAtLocation : public UBFNode_Base
{
	GENERATED_UCLASS_BODY()

	/** 要生成的 Actor 类 */
	UPROPERTY(EditAnywhere, Category = "SpawnActor")
	TSubclassOf<AActor> ActorClass;

	/** 是否使用 BFC->LastKillLocation 作为生成位置 */
	UPROPERTY(EditAnywhere, Category = "SpawnActor")
	bool bUseKillLocation = true;

	/** 在基准位置上的额外偏移 */
	UPROPERTY(EditAnywhere, Category = "SpawnActor")
	FVector LocationOffset = FVector::ZeroVector;

protected:
	virtual void ExecuteInput(const FName& PinName) override;
};
