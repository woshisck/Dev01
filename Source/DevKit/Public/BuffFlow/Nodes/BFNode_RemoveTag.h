#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "BuffFlow/Nodes/BFNode_Base.h"
#include "BuffFlow/BuffFlowTypes.h"
#include "BFNode_RemoveTag.generated.h"

UCLASS(NotBlueprintable, meta = (DisplayName = "Remove Tag", Category = "BuffFlow|Tag"))
class DEVKIT_API UBFNode_RemoveTag : public UBFNode_Base
{
	GENERATED_UCLASS_BODY()

	// 要移除的 Gameplay Tag
	UPROPERTY(EditAnywhere, Category = "BuffFlow", meta = (DisplayName = "移除 Tag"))
	FGameplayTag Tag;

	// 移除层数 — 每次触发 In 引脚时移除几层此 Tag
	UPROPERTY(EditAnywhere, Category = "BuffFlow", meta = (DisplayName = "层数"))
	int32 Count = 1;

	// 目标 — 在哪个 Actor 的 ASC 上移除 Tag，默认 BuffOwner
	UPROPERTY(EditAnywhere, Category = "BuffFlow", meta = (DisplayName = "目标"))
	EBFTargetSelector Target = EBFTargetSelector::BuffOwner;

protected:
	virtual void ExecuteBuffFlowInput(const FName& PinName) override;
};
