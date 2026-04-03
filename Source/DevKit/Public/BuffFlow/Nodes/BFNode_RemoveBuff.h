#pragma once

#include "CoreMinimal.h"
#include "BuffFlow/Nodes/BFNode_Base.h"
#include "BuffFlow/BuffFlowTypes.h"
#include "BFNode_RemoveBuff.generated.h"

class UYogBuffDefinition;

UCLASS(NotBlueprintable, meta = (DisplayName = "移除Buff", Category = "BuffFlow|增益"))
class DEVKIT_API UBFNode_RemoveBuff : public UBFNode_Base
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, Category = "BuffFlow")
	TObjectPtr<UYogBuffDefinition> BuffDefinition;

	UPROPERTY(EditAnywhere, Category = "BuffFlow")
	EBFTargetSelector Target = EBFTargetSelector::BuffOwner;

protected:
	virtual void ExecuteInput(const FName& PinName) override;
};
