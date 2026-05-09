#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "BuffFlow/Nodes/BFNode_Base.h"
#include "BuffFlow/BuffFlowTypes.h"
#include "BFNode_SpawnGameplayCueOnActor.generated.h"

/**
 * 在目标 Actor 身上触发一次性 GameplayCue（Execute 语义）。
 *
 * Out    — 执行成功
 * Failed — CueTag 无效或目标无 ASC
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "Spawn Gameplay Cue on Actor", Category = "BuffFlow|Visual"))
class DEVKIT_API UBFNode_SpawnGameplayCueOnActor : public UBFNode_Base
{
	GENERATED_UCLASS_BODY()

	// GameplayCue Tag — 要触发的 GC Tag（GC 资产需在 GC Manager 中注册此 Tag）
	UPROPERTY(EditAnywhere, Category = "GameplayCue", meta = (DisplayName = "Cue Tag"))
	FGameplayTag CueTag;

	// 附着目标 — GC 在此 Actor 的 ASC 上执行
	UPROPERTY(EditAnywhere, Category = "GameplayCue", meta = (DisplayName = "附着目标"))
	EBFTargetSelector Target = EBFTargetSelector::BuffOwner;

protected:
	virtual void ExecuteInput(const FName& PinName) override;
};
