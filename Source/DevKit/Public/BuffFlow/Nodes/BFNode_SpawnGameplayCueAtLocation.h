#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "BuffFlow/Nodes/BFNode_Base.h"
#include "BuffFlow/BuffFlowTypes.h"
#include "BFNode_SpawnGameplayCueAtLocation.generated.h"

/**
 * 在指定世界位置触发一次性 GameplayCue（Execute 语义）。
 *
 * 位置来源（二选一）：
 *  · bUseKillLocation = true  → BFC->LastKillLocation + LocationOffset
 *  · bUseKillLocation = false → LocationSource 指定 Actor 的位置 + LocationOffset
 *
 * Out    — 执行成功
 * Failed — CueTag 无效或位置解析失败
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "Spawn Gameplay Cue at Location", Category = "BuffFlow|Visual"))
class DEVKIT_API UBFNode_SpawnGameplayCueAtLocation : public UBFNode_Base
{
	GENERATED_UCLASS_BODY()

	// GameplayCue Tag — 要触发的 GC Tag（GC 资产需注册此 Tag）
	UPROPERTY(EditAnywhere, Category = "GameplayCue", meta = (DisplayName = "Cue Tag"))
	FGameplayTag CueTag;

	// 位置来源 — 取此 Actor 的位置作为 GC 触发点（不使用击杀位置时生效）
	UPROPERTY(EditAnywhere, Category = "GameplayCue", meta = (EditCondition = "!bUseKillLocation", DisplayName = "位置来源"))
	EBFTargetSelector LocationSource = EBFTargetSelector::LastDamageTarget;

	// 使用击杀位置 — 勾选后使用 BFC->LastKillLocation（由击杀时节点写入）
	UPROPERTY(EditAnywhere, Category = "GameplayCue", meta = (DisplayName = "使用击杀位置"))
	bool bUseKillLocation = false;

	// 位置偏移 — 在基准位置上的额外世界坐标偏移
	UPROPERTY(EditAnywhere, Category = "GameplayCue", meta = (DisplayName = "位置偏移"))
	FVector LocationOffset = FVector::ZeroVector;

protected:
	virtual void ExecuteBuffFlowInput(const FName& PinName) override;
};
