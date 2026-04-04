#pragma once

#include "CoreMinimal.h"
#include "BuffFlow/Nodes/BFNode_Base.h"
#include "BuffFlow/BuffFlowTypes.h"
#include "GameplayTagContainer.h"
#include "BFNode_AddEffect.generated.h"

/**
 * Add Effect by Tag
 * Queries the global EffectRegistry (configured in Project Settings → Yog)
 *
 * Out    — effect applied successfully
 * Failed — tag not registered, or target has no ASC
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "Add Effect (Tag)", Category = "BuffFlow|Effect"))
class DEVKIT_API UBFNode_AddEffect : public UBFNode_Base
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, Category = "BuffFlow")
	FGameplayTag EffectTag;

	UPROPERTY(EditAnywhere, Category = "BuffFlow")
	EBFTargetSelector Target = EBFTargetSelector::BuffOwner;

	UPROPERTY(EditAnywhere, Category = "BuffFlow")
	int32 Level = 1;

protected:
	virtual void ExecuteInput(const FName& PinName) override;
};
