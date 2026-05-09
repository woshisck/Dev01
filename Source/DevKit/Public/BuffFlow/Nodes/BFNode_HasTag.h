#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "BuffFlow/Nodes/BFNode_Base.h"
#include "BuffFlow/BuffFlowTypes.h"
#include "BFNode_HasTag.generated.h"

UCLASS(NotBlueprintable, meta = (DisplayName = "Has Tag", Category = "BuffFlow|Tag"))
class DEVKIT_API UBFNode_HasTag : public UBFNode_Base
{
	GENERATED_UCLASS_BODY()

	// 要检查的 Gameplay Tag — 目标拥有此 Tag 走 True 引脚，否则走 False
	UPROPERTY(EditAnywhere, Category = "BuffFlow", meta = (DisplayName = "检查 Tag"))
	FGameplayTag Tag;

	// 检查目标 — 在哪个 Actor 的 ASC 上检查 Tag，默认 BuffOwner
	UPROPERTY(EditAnywhere, Category = "BuffFlow", meta = (DisplayName = "检查目标"))
	EBFTargetSelector Target = EBFTargetSelector::BuffOwner;

protected:
	virtual void ExecuteInput(const FName& PinName) override;
};
