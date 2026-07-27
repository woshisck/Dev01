#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "StylizedEmissiveBatchSubsystem.generated.h"

class AActor;
class ULevel;

/**
 * Builds transient HISM components for compatible Stylized Emissive Source
 * actors. Source actors remain alive as Blueprint configuration and light-data
 * providers; only their individual visible meshes are replaced.
 */
UCLASS(DisplayName = "Stylized Emissive Batch Subsystem")
class CELESLIGHTRUNTIME_API UStylizedEmissiveBatchSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;

	/** Rebuilds batches for every currently loaded level. */
	UFUNCTION(BlueprintCallable, Category = "Stylized Emissive|Performance")
	void RebuildAllBatches();

	/** Restores individual source meshes and destroys all transient batches. */
	UFUNCTION(BlueprintCallable, Category = "Stylized Emissive|Performance")
	void ClearAllBatches();

	UFUNCTION(BlueprintPure, Category = "Stylized Emissive|Performance")
	int32 GetBatchedSourceCount() const { return BatchedSourceCount; }

	UFUNCTION(BlueprintPure, Category = "Stylized Emissive|Performance")
	int32 GetBatchComponentCount() const { return BatchComponentCount; }

protected:
	virtual bool DoesSupportWorldType(EWorldType::Type WorldType) const override;

private:
	void RebuildLevelBatch(ULevel* Level);
	void ClearLevelBatch(ULevel* Level, bool bRestoreSources);
	void HandleLevelAddedToWorld(ULevel* Level, UWorld* World);
	void HandleLevelRemovedFromWorld(ULevel* Level, UWorld* World);

	UPROPERTY(Transient)
	TArray<TObjectPtr<AActor>> BatchActors;

	UPROPERTY(Transient)
	int32 BatchedSourceCount = 0;

	UPROPERTY(Transient)
	int32 BatchComponentCount = 0;

	FDelegateHandle LevelAddedHandle;
	FDelegateHandle LevelRemovedHandle;
};
