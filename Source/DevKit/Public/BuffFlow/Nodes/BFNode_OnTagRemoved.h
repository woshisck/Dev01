#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "BuffFlow/Nodes/BFNode_Base.h"
#include "BuffFlow/BuffFlowTypes.h"
#include "BFNode_OnTagRemoved.generated.h"

/**
 * BFNode_OnTagRemoved — 监听 Gameplay Tag 被移除
 *
 * In   → 开始监听；每次指定 Tag 的计数从 >=1 降到 0 时触发 Out
 * Stop → 停止监听（FA 停止时 Cleanup 也会自动解绑）
 *
 * 使用 EGameplayTagEventType::NewOrRemoved，仅在 0↔非零边沿触发。
 */
class UAbilitySystemComponent;

UCLASS(NotBlueprintable, meta = (DisplayName = "On Tag Removed", Category = "BuffFlow|Tag"))
class DEVKIT_API UBFNode_OnTagRemoved : public UBFNode_Base
{
	GENERATED_UCLASS_BODY()

	// 要监听的 Gameplay Tag — 当此 Tag 从有到无（≥1→0）时触发 Out
	UPROPERTY(EditAnywhere, Category = "BuffFlow", meta = (DisplayName = "监听 Tag"))
	FGameplayTag Tag;

	// 监听目标 — 在哪个 Actor 的 ASC 上监听此 Tag，默认为 BuffOwner
	UPROPERTY(EditAnywhere, Category = "BuffFlow", meta = (DisplayName = "监听目标"))
	EBFTargetSelector Target = EBFTargetSelector::BuffOwner;

protected:
	virtual void ExecuteBuffFlowInput(const FName& PinName) override;
	virtual void Cleanup() override;

private:
	void HandleTagChanged(const FGameplayTag InTag, int32 NewCount);
	void Unbind();

	TWeakObjectPtr<UAbilitySystemComponent> BoundASC;
	FDelegateHandle TagDelegateHandle;
};
