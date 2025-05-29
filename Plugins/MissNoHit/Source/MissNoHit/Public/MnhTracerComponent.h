// Copyright 2024 Eren Balatkan. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Components/ActorComponent.h"
#include "Engine/HitResult.h"
#include "MnhTracerComponent.generated.h"

DECLARE_STATS_GROUP(TEXT("MissNoHit"), STATGROUP_MISSNOHIT, STATCAT_Advanced)
DECLARE_CYCLE_STAT(TEXT("MissNoHit Start Tracer"), STAT_MNHStartTracer, STATGROUP_MISSNOHIT);
DECLARE_CYCLE_STAT(TEXT("MissNoHit Check Filters"), STAT_MNHCheckFilters, STATGROUP_MISSNOHIT);
DECLARE_CYCLE_STAT(TEXT("MissNoHit Add Tracer"), STAT_MnhAddTracer, STATGROUP_MISSNOHIT);
DECLARE_CYCLE_STAT(TEXT("MissNoHit Remove Tracer"), STAT_MnhRemoveTracer, STATGROUP_MISSNOHIT);
DECLARE_CYCLE_STAT(TEXT("MissNoHit Tick Tracers"), STAT_MnhTickTracers, STATGROUP_MISSNOHIT);
DECLARE_CYCLE_STAT(TEXT("MissNoHit Tracer Update Transforms"), STAT_MnhTracerUpdateTransforms, STATGROUP_MISSNOHIT);
DECLARE_CYCLE_STAT(TEXT("MissNoHit Tracer Do Trace"), STAT_MnhTracerDoTrace, STATGROUP_MISSNOHIT);
DECLARE_CYCLE_STAT(TEXT("MissNoHit Tracer Debug Draw"), STAT_MnhTracerDebugDraw, STATGROUP_MISSNOHIT);
DECLARE_CYCLE_STAT(TEXT("MissNoHit Tracer Trace Done"), STAT_MnhTracerTraceDone, STATGROUP_MISSNOHIT);
DECLARE_CYCLE_STAT(TEXT("MissNoHit TracerComponent Hit Detected"), STAT_MnhTracerComponentHitDetected, STATGROUP_MISSNOHIT);
DECLARE_CYCLE_STAT(TEXT("MissNoHit Tracer Control Node Hit Detected "), STAT_MnhTracerHitDetectedAsyncNode, STATGROUP_MISSNOHIT);
DECLARE_CYCLE_STAT(TEXT("MissNoHit Tracer Get Shape"), STAT_MnhGetTracerShape, STATGROUP_MISSNOHIT)

DECLARE_LOG_CATEGORY_EXTERN(LogMnh, Log, All)

struct FGameplayTagContainer;
class UMnhTracer;
struct FGameplayTag;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FMnhOnHitDetected, FGameplayTag, TracerTag, FHitResult, HitResult, float, DeltaTime);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FMnhOnTraceFinished, FGameplayTag, TracerTag, const TArray<FHitResult>&, HitResult, const float, DeltaTime);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMnhOnTracerStopped, FGameplayTag, TracerTag);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMnhOnTracerStarted, FGameplayTag, TracerTag);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMnhTracerComponentDestroyed);

struct FMnhHitCache
{
	FHitResult HitResult;
	FGameplayTag TracerTag;
	int TickIdx;
};

struct FTracerInitializationData
{
	FGameplayTagContainer TracerTags;
	UPrimitiveComponent* TracerSource;
};

UENUM(BlueprintType)
enum class EMnhFilterType : uint8
{
    FilterSameActorAcrossAllTracers UMETA(DisplayName="FilterSameActorAcrossAllTracers"),
    FilterSameActorPerTracer UMETA(DisplayName="FilterSameActorPerTracer"),
	None UMETA(DisplayName="None")
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent),
	HideCategories=(Sockets, Navigation, Tags, ComponentTick, ComponentReplication,
		Cooking, AssetUserData, Replication),
		meta=(PrioritizeCategories="Mnh Hit Tracers"), CollapseCategories)
class MISSNOHIT_API UMnhTracerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UMnhTracerComponent();
	virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override;

	FMnhOnHitDetected OnHitDetected;
	FMnhOnTracerStarted OnTracerStarted;
	FMnhOnTracerStopped OnTracerStopped;
	FMnhTracerComponentDestroyed OnDestroyed;

	FCriticalSection TraceDoneScopeLock;

	TMap<int, TArray<FMnhHitCache>> HitCache;
	
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category="MissNoHit", meta=(FullyExpand=true))
	TArray<TObjectPtr<UMnhTracer>> Tracers;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="MissNoHit")
	EMnhFilterType FilterType;

	UFUNCTION(BlueprintCallable, Category="MissNoHit")
	void InitializeTracers(const FGameplayTagContainer TracerTags, UPrimitiveComponent* TracerSource);

	UFUNCTION(BlueprintCallable, Category="MissNoHit")
	void StopAllTracers();

	UFUNCTION(BlueprintCallable, Category="MissNoHit")
	void ResetHitCache();

	UFUNCTION(BlueprintCallable, Category="MissNoHit")
	void StartTracers(FGameplayTagContainer TracerTags, bool bResetHitCache=true);

	template <bool AllowAnimNotify>
	void StartTracersInternal(const FGameplayTagContainer& TracerTags, bool bResetHitCache);

	UFUNCTION(BlueprintCallable, Category="MissNoHit")
	void StopTracers(FGameplayTagContainer TracerTags);

	void StopTracersDelayed(const FGameplayTagContainer& TracerTags);

	UMnhTracer* FindTracer(FGameplayTag TracerTag);

protected:
	// Called when the game starts
	virtual void BeginPlay() override;
	virtual bool CheckFilters(const FHitResult& HitResult, FGameplayTag TracerTag, int TickIdxArg);

public:
	bool AreAllTracersRunning(const FGameplayTagContainer& TracerTags);
	bool AreAllTracersStopped(const FGameplayTagContainer& TracerTags);
	void OnTracerHitDetected(FGameplayTag TracerTag, const TArray<FHitResult>& HitResults, const float DeltaTime, const int TickIdx);

private:
	bool bIsInitialized = false;
	TArray<FTracerInitializationData> EarlyTracerInitializations;
};
