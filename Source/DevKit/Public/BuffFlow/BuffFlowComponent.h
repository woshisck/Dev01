#pragma once

#include "CoreMinimal.h"
#include "FlowComponent.h"
#include "BuffFlowComponent.generated.h"

class UYogAbilitySystemComponent;
class AYogCharacterBase;
class UFlowAsset;

/**
 * 挂在角色上的 BuffFlow 管理组件
 * - 管理 FlowAsset 实例的生命周期（符文激活→启动 / 卸下→停止）
 * - 为所有 BuffFlow 节点提供 Context（BuffOwner, BuffGiver, ASC）
 */
UCLASS(BlueprintType, meta = (BlueprintSpawnableComponent))
class DEVKIT_API UBuffFlowComponent : public UFlowComponent
{
	GENERATED_BODY()

public:
	UBuffFlowComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// ─── 符文 Flow 管理 ─────────────────────────────────────

	/** 符文激活时调用：启动一个 Flow 实例 */
	UFUNCTION(BlueprintCallable, Category = "BuffFlow")
	void StartBuffFlow(UFlowAsset* FlowAsset, FGuid RuneGuid, AActor* Giver);

	/** 符文卸下时调用：停止对应的 Flow 实例 */
	UFUNCTION(BlueprintCallable, Category = "BuffFlow")
	void StopBuffFlow(FGuid RuneGuid);

	/** 停止所有活跃的 BuffFlow */
	UFUNCTION(BlueprintCallable, Category = "BuffFlow")
	void StopAllBuffFlows();

	// ─── 供节点快速访问 ─────────────────────────────────────

	UFUNCTION(BlueprintPure, Category = "BuffFlow")
	UYogAbilitySystemComponent* GetASC() const;

	UFUNCTION(BlueprintPure, Category = "BuffFlow")
	AYogCharacterBase* GetBuffOwner() const;

	UFUNCTION(BlueprintPure, Category = "BuffFlow")
	AActor* GetBuffGiver() const;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UPROPERTY()
	TWeakObjectPtr<UYogAbilitySystemComponent> CachedASC;

	UPROPERTY()
	TWeakObjectPtr<AActor> CurrentBuffGiver;

	/** RuneGuid → 活跃的 Flow 实例（用于停止） */
	TMap<FGuid, TWeakObjectPtr<UFlowAsset>> ActiveRuneFlows;
};
