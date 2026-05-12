#pragma once

#include "CoreMinimal.h"
#include "FlowComponent.h"
#include "BuffFlow/BuffFlowTypes.h"
#include "BuffFlowComponent.generated.h"

class UYogAbilitySystemComponent;
class AYogCharacterBase;
class UFlowAsset;
class UNiagaraComponent;

/** Buff Flow 启动/停止事件：携带 RuneGuid 供监听节点过滤 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBuffFlowEvent, FGuid, RuneGuid);

UENUM(BlueprintType)
enum class EBuffFlowTraceResult : uint8
{
	Success UMETA(DisplayName = "Success"),
	Failed UMETA(DisplayName = "Failed"),
	Skipped UMETA(DisplayName = "Skipped")
};

USTRUCT(BlueprintType)
struct DEVKIT_API FBuffFlowTraceEntry
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "BuffFlow|Trace")
	float TimeSeconds = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "BuffFlow|Trace")
	FName FlowName = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "BuffFlow|Trace")
	FName NodeName = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "BuffFlow|Trace")
	FName NodeClass = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "BuffFlow|Trace")
	FName ProfileName = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "BuffFlow|Trace")
	FName OwnerName = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "BuffFlow|Trace")
	FName TargetName = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "BuffFlow|Trace")
	FName CardName = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "BuffFlow|Trace")
	FGameplayTag CardIdTag;

	UPROPERTY(BlueprintReadOnly, Category = "BuffFlow|Trace")
	EBuffFlowTraceResult Result = EBuffFlowTraceResult::Success;

	UPROPERTY(BlueprintReadOnly, Category = "BuffFlow|Trace")
	FString Message;

	UPROPERTY(BlueprintReadOnly, Category = "BuffFlow|Trace")
	FString Values;
};

USTRUCT(BlueprintType)
struct DEVKIT_API FBuffFlowActiveFlowDebugEntry
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "BuffFlow|Debug")
	FGuid RuneGuid;

	UPROPERTY(BlueprintReadOnly, Category = "BuffFlow|Debug")
	FName FlowName = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "BuffFlow|Debug")
	FName SourceRuneName = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "BuffFlow|Debug")
	bool bFlowAssetValid = false;

	UPROPERTY(BlueprintReadOnly, Category = "BuffFlow|Debug")
	bool bRuntimeInstanceActive = false;

	UPROPERTY(BlueprintReadOnly, Category = "BuffFlow|Debug")
	int32 ActiveNodeCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "BuffFlow|Debug")
	int32 RecordedNodeCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "BuffFlow|Debug")
	TArray<FName> ActiveNodeNames;

	UPROPERTY(BlueprintReadOnly, Category = "BuffFlow|Debug")
	TArray<FName> ActiveNodeClasses;

	UPROPERTY(BlueprintReadOnly, Category = "BuffFlow|Debug")
	TArray<FName> RecordedNodeNames;
};

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

	/** 符文卸下时调用：停止对应的 Flow 实例 */
	UFUNCTION(BlueprintCallable, Category = "BuffFlow")
	void StopBuffFlow(FGuid RuneGuid);

	/** 停止所有活跃的 BuffFlow */
	UFUNCTION(BlueprintCallable, Category = "BuffFlow")
	void StopAllBuffFlows();

	// ─── 调试 / Trace 检视 ─────────────────────────────────────

	UFUNCTION(BlueprintCallable, Category = "BuffFlow|Trace")
	void ClearTraceEntries();

	UFUNCTION(BlueprintPure, Category = "BuffFlow|Trace")
	TArray<FBuffFlowTraceEntry> GetTraceEntries() const { return TraceEntries; }

	UFUNCTION(BlueprintPure, Category = "BuffFlow|Debug")
	int32 GetActiveBuffFlowCount() const;

	UFUNCTION(BlueprintPure, Category = "BuffFlow|Debug")
	TArray<FBuffFlowActiveFlowDebugEntry> GetActiveBuffFlowDebugEntries() const;

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

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	/** 最近一次伤害事件的上下文（由 OnDamageDealt / OnDamageReceived 写入，供后续节点读取） */
	UPROPERTY()
	FBFEventContext LastEventContext;

	/** 由 PlayNiagara 节点写入，DestroyNiagara 节点读取，key = 策划自定义名称 */
	UPROPERTY()
	TMap<FName, TObjectPtr<UNiagaraComponent>> ActiveNiagaraEffects;

	/** 由 BFNode_OnKill 写入，BFNode_SpawnActorAtLocation 读取 */
	UPROPERTY()
	FVector LastKillLocation = FVector::ZeroVector;

private:
	UPROPERTY()
	TWeakObjectPtr<UYogAbilitySystemComponent> CachedASC;

	UPROPERTY()
	TWeakObjectPtr<AActor> CurrentBuffGiver;

	/** RuneGuid → 活跃的 Flow 实例（用于停止） */
	TMap<FGuid, TWeakObjectPtr<UFlowAsset>> ActiveRuneFlows;

	UPROPERTY()
	TArray<FBuffFlowTraceEntry> TraceEntries;

};
