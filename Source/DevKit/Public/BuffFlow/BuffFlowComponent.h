#pragma once

#include "CoreMinimal.h"
#include "FlowComponent.h"
#include "BuffFlow/BuffFlowTypes.h"
#include "Component/CombatDeckComponent.h"
#include "BuffFlowComponent.generated.h"

class UYogAbilitySystemComponent;
class AYogCharacterBase;
class UFlowAsset;
class UNiagaraComponent;

/** Buff Flow 启动/停止事件：携带 RuneGuid 供监听节点过滤 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBuffFlowEvent, FGuid, RuneGuid);

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
	void StartBuffFlow(UFlowAsset* FlowAsset, FGuid RuneGuid, AActor* Giver, bool bRestartExistingFlow = false);

	void StartCombatCardFlow(UFlowAsset* FlowAsset, const FCombatCardInstance& Card, const FCombatDeckActionContext& ActionContext, const FCombatCardResolveResult& ResolveResult, AActor* Giver, bool bRestartExistingFlow = true);

	void StartCombatCardFlowWithSourceTransform(
		UFlowAsset* FlowAsset,
		const FCombatCardInstance& Card,
		const FCombatDeckActionContext& ActionContext,
		const FCombatCardResolveResult& ResolveResult,
		AActor* Giver,
		const FTransform& SourceTransform,
		bool bRestartExistingFlow = true);

	bool GetActiveSourceTransformOverride(FTransform& OutTransform) const;

	/** 符文卸下时调用：停止对应的 Flow 实例 */
	UFUNCTION(BlueprintCallable, Category = "BuffFlow")
	void StopBuffFlow(FGuid RuneGuid);

	/** 停止所有活跃的 BuffFlow */
	UFUNCTION(BlueprintCallable, Category = "BuffFlow")
	void StopAllBuffFlows();

	UFlowAsset* GetActiveBuffFlowAsset(FGuid RuneGuid) const;

	// ─── Buff 事件委托 ─────────────────────────────────────

	/** 有新 BuffFlow 启动时广播 */
	UPROPERTY(BlueprintAssignable, Category = "BuffFlow")
	FOnBuffFlowEvent OnBuffFlowStarted;

	/** 有 BuffFlow 停止时广播 */
	UPROPERTY(BlueprintAssignable, Category = "BuffFlow")
	FOnBuffFlowEvent OnBuffFlowStopped;

	// ─── 供节点快速访问 ─────────────────────────────────────

	UFUNCTION(BlueprintPure, Category = "BuffFlow")
	UYogAbilitySystemComponent* GetASC() const;

	UFUNCTION(BlueprintPure, Category = "BuffFlow")
	AYogCharacterBase* GetBuffOwner() const;

	UFUNCTION(BlueprintPure, Category = "BuffFlow")
	AActor* GetBuffGiver() const;

	UFUNCTION(BlueprintPure, Category = "BuffFlow|Combat Card")
	bool HasCombatCardEffectContext() const { return bHasCombatCardEffectContext; }

	const FCombatCardEffectContext& GetLastCombatCardEffectContext() const { return LastCombatCardEffectContext; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	/** 最近一次伤害事件的上下文（由 OnDamageDealt / OnDamageReceived 写入，供后续节点读取） */
	UPROPERTY()
	FBFEventContext LastEventContext;

	UPROPERTY()
	FCombatCardEffectContext LastCombatCardEffectContext;

	/** 由 PlayNiagara 节点写入，DestroyNiagara 节点读取，key = 策划自定义名称 */
	UPROPERTY()
	TMap<FName, TObjectPtr<UNiagaraComponent>> ActiveNiagaraEffects;

	/** 由 BFNode_OnKill 写入，BFNode_SpawnActorAtLocation 读取 */
	UPROPERTY()
	FVector LastKillLocation = FVector::ZeroVector;

private:
	void StartBuffFlowInternal(UFlowAsset* FlowAsset, FGuid RuneGuid, AActor* Giver, bool bRestartExistingFlow, bool bAllowParallelSameFlow = false);

	UPROPERTY()
	bool bHasCombatCardEffectContext = false;

	UPROPERTY()
	TWeakObjectPtr<UYogAbilitySystemComponent> CachedASC;

	UPROPERTY()
	TWeakObjectPtr<AActor> CurrentBuffGiver;

	bool bHasSourceTransformOverride = false;
	FTransform SourceTransformOverride = FTransform::Identity;

	/** RuneGuid → 活跃的 Flow 实例（用于停止） */
	TMap<FGuid, TWeakObjectPtr<UFlowAsset>> ActiveRuneFlows;

};
