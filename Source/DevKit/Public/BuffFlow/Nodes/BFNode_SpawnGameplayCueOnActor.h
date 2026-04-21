#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "BuffFlow/Nodes/BFNode_Base.h"
#include "BuffFlow/BuffFlowTypes.h"
#include "BFNode_SpawnGameplayCueOnActor.generated.h"

/**
 * 在目标 Actor 身上触发一次性 GameplayCue（Execute 语义）。
 *
 * 典型用法：FA_Rune_LifeSteal 在回血后播放绿色光效
 *   CueTag = GameplayCue.Rune.LifeSteal
 *   Target = BuffOwner
 *
 * Out    — 执行成功
 * Failed — CueTag 无效或目标无 ASC
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "Spawn Gameplay Cue on Actor", Category = "BuffFlow|Visual"))
class DEVKIT_API UBFNode_SpawnGameplayCueOnActor : public UBFNode_Base
{
	GENERATED_UCLASS_BODY()

	/** GameplayCue Tag（GC 资产需在 GC Manager 中注册此 Tag） */
	UPROPERTY(EditAnywhere, Category = "GameplayCue")
	FGameplayTag CueTag;

	/** Cue 附着的目标 Actor（GC 在此 Actor 的 ASC 上执行） */
	UPROPERTY(EditAnywhere, Category = "GameplayCue")
	EBFTargetSelector Target = EBFTargetSelector::BuffOwner;

protected:
	virtual void ExecuteInput(const FName& PinName) override;
};
