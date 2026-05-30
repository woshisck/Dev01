#pragma once

#include "CoreMinimal.h"
#include "Types/FlowDataPinProperties.h"
#include "BuffFlow/Nodes/BFNode_Base.h"
#include "BuffFlow/BuffFlowTypes.h"
#include "GameplayEffect.h"
#include "BFNode_RemoveGE.generated.h"

/**
 * Removes all active instances of a GameplayEffect class from the target's ASC,
 * if any exist. Useful for cancelling a timed GE early or cleaning up a status
 * effect before re-applying a fresh one.
 *
 * Target — which actor to search. Supports all EBFTargetSelector values,
 *           including AllHitTargets (removes from every target in the hit list).
 *
 * RemoveMode controls how many stacks are removed per target:
 *   AllStacks   — removes every active instance of the GE class (default)
 *   OneStack    — removes one stack, leaving the rest intact
 *   CustomCount — removes exactly StacksToRemove stacks (data-pin driven)
 *
 * Output pins:
 *   Removed  — at least one stack was removed from at least one target
 *   NotFound — the GE was not active on any resolved target
 *   Failed   — Effect class is unset, or target has no ASC
 *
 * Output data pins:
 *   RemovedCount — total stacks removed across all targets
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "Remove Gameplay Effect", Category = "BuffFlow|Effect"))
class DEVKIT_API UBFNode_RemoveGE : public UBFNode_Base
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, Category = "Effect")
	TSubclassOf<UGameplayEffect> Effect;

	UPROPERTY(EditAnywhere, Category = "Effect")
	EBFTargetSelector Target = EBFTargetSelector::LastDamageTarget;

	UPROPERTY(EditAnywhere, Category = "Remove")
	EBFRemoveMode RemoveMode = EBFRemoveMode::AllStacks;

	UPROPERTY(EditAnywhere, Category = "Remove",
		meta = (EditCondition = "RemoveMode == EBFRemoveMode::CustomCount", EditConditionHides))
	FFlowDataPinInputProperty_Int32 StacksToRemove;

	UPROPERTY(EditAnywhere, Category = "Output")
	FFlowDataPinOutputProperty_Int32 RemovedCount;

protected:
	virtual void ExecuteBuffFlowInput(const FName& PinName) override;
};
