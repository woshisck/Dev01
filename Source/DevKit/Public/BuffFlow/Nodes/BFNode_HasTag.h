#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "BuffFlow/Nodes/BFNode_Base.h"
#include "BuffFlow/BuffFlowTypes.h"
#include "BFNode_HasTag.generated.h"

UCLASS(NotBlueprintable, meta = (DisplayName = "检查标签", Category = "BuffFlow|标签"))
class DEVKIT_API UBFNode_HasTag : public UBFNode_Base
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, Category = "BuffFlow")
	FGameplayTag Tag;

	UPROPERTY(EditAnywhere, Category = "BuffFlow")
	EBFTargetSelector Target = EBFTargetSelector::BuffOwner;

protected:
	virtual void ExecuteInput(const FName& PinName) override;
};
