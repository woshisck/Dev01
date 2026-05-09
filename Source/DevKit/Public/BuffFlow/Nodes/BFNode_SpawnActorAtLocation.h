#pragma once

#include "CoreMinimal.h"
#include "BuffFlow/Nodes/BFNode_Base.h"
#include "BFNode_SpawnActorAtLocation.generated.h"

/**
 * 即时节点：在指定位置生成 Actor
 * 默认使用 BFC->LastKillLocation（由 BFNode_OnKill 写入）
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "Spawn Actor At Location", Category = "BuffFlow|Spawn"))
class DEVKIT_API UBFNode_SpawnActorAtLocation : public UBFNode_Base
{
	GENERATED_UCLASS_BODY()

	// 要生成的 Actor 类
	UPROPERTY(EditAnywhere, Category = "SpawnActor", meta = (DisplayName = "Actor 类"))
	TSubclassOf<AActor> ActorClass;

	// 使用击杀位置 — 勾选后在 BFC->LastKillLocation 处生成，否则在拥有者位置 + 偏移处生成
	UPROPERTY(EditAnywhere, Category = "SpawnActor", meta = (DisplayName = "使用击杀位置"))
	bool bUseKillLocation = true;

	// 位置偏移 — 在基准位置上的额外世界坐标偏移
	UPROPERTY(EditAnywhere, Category = "SpawnActor", meta = (DisplayName = "位置偏移"))
	FVector LocationOffset = FVector::ZeroVector;

protected:
	virtual void ExecuteInput(const FName& PinName) override;
};
