#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "BuffFlow/Nodes/BFNode_Base.h"
#include "BuffFlow/BuffFlowTypes.h"
#include "BFNode_AddTag.generated.h"

class UYogAbilitySystemComponent;

UCLASS(NotBlueprintable, meta = (DisplayName = "Add Tag", Category = "BuffFlow|Tag"))
class DEVKIT_API UBFNode_AddTag : public UBFNode_Base
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

	/** FA 停止时自动移除此节点添加的所有 Tag（累计层数） */
	virtual void Cleanup() override;

private:
	/** 累计已添加的层数（触发型节点可能多次执行） */
	int32 TotalCountAdded = 0;
	TWeakObjectPtr<UYogAbilitySystemComponent> StoredASC;
};
