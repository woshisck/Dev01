#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "BuffFlow/Nodes/BFNode_Base.h"
#include "BuffFlow/BuffFlowTypes.h"
#include "BFNode_RemoveTag.generated.h"

UCLASS(NotBlueprintable, meta = (DisplayName = "移除标签", Category = "BuffFlow|标签"))
class DEVKIT_API UBFNode_RemoveTag : public UBFNode_Base
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, Category = "BuffFlow")
	FGameplayTag Tag;

	UPROPERTY(EditAnywhere, Category = "BuffFlow")
	int32 Count = 1;

	UPROPERTY(EditAnywhere, Category = "BuffFlow")
	EBFTargetSelector Target = EBFTargetSelector::BuffOwner;

protected:
	virtual void ExecuteInput(const FName& PinName) override;
};
