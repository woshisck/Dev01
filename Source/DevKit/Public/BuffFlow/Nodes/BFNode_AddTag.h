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

	// 要添加的 Gameplay Tag — FA 停止时由 Cleanup() 自动移除
	UPROPERTY(EditAnywhere, Category = "BuffFlow", meta = (DisplayName = "添加 Tag"))
	FGameplayTag Tag;

	// 添加层数 — 每次触发 In 引脚时添加几层此 Tag
	UPROPERTY(EditAnywhere, Category = "BuffFlow", meta = (DisplayName = "层数"))
	int32 Count = 1;

	// 目标 — 在哪个 Actor 的 ASC 上添加 Tag，默认 BuffOwner
	UPROPERTY(EditAnywhere, Category = "BuffFlow", meta = (DisplayName = "目标"))
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
