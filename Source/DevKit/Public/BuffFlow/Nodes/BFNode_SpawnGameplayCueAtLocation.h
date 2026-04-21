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
 * 典型用法：
 *  · FA_Rune_KillExplosion：在死亡位置播放爆炸 GC
 *      LocationSource=LastDamageTarget, CueTag=GameplayCue.Rune.KillExplosion
 *  · FA_Rune_Aftershock：在击退落点播放震荡 GC
 *      LocationSource=LastDamageTarget, CueTag=GameplayCue.Rune.Aftershock
 *
 * Out    — 执行成功
 * Failed — CueTag 无效或位置解析失败
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "Spawn Gameplay Cue at Location", Category = "BuffFlow|Visual"))
class DEVKIT_API UBFNode_SpawnGameplayCueAtLocation : public UBFNode_Base
{
	GENERATED_UCLASS_BODY()

	/** GameplayCue Tag */
	UPROPERTY(EditAnywhere, Category = "GameplayCue")
	FGameplayTag CueTag;

	/**
	 * 位置来源 Actor（取其 GetActorLocation()）。
	 * 仅在 bUseKillLocation = false 时生效。
	 */
	UPROPERTY(EditAnywhere, Category = "GameplayCue",
		meta = (EditCondition = "!bUseKillLocation"))
	EBFTargetSelector LocationSource = EBFTargetSelector::LastDamageTarget;

	/** 使用 BFC->LastKillLocation 作为基准位置（由 BFNode_OnKill 写入） */
	UPROPERTY(EditAnywhere, Category = "GameplayCue")
	bool bUseKillLocation = false;

	/** 在基准位置上的额外偏移 */
	UPROPERTY(EditAnywhere, Category = "GameplayCue")
	FVector LocationOffset = FVector::ZeroVector;

protected:
	virtual void ExecuteInput(const FName& PinName) override;
};
