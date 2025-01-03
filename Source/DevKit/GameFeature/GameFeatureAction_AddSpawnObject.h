// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Containers/Array.h"
#include "Math/Transform.h"
#include "Misc/CoreMiscDefines.h"
#include "Templates/SubclassOf.h"
#include "UObject/SoftObjectPtr.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/WeakObjectPtrTemplates.h"

#include "GameFeatureAction_WorldActionBase.h"
#include "GameFeatureAction_AddSpawnObject.generated.h"


class AActor;
class FText;
class UObject;
class UWorld;
struct FAssetBundleData;
struct FGameFeatureDeactivatingContext;
struct FWorldContext;

USTRUCT()
struct FSpawningActorEntry
{
	GENERATED_BODY()

	// What kind of actor to spawn
	UPROPERTY(EditAnywhere, Category = "Actor")
	TSubclassOf<AActor> ActorType;

	// Where to spawn the actor
	UPROPERTY(EditAnywhere, Category = "Actor|Transform")
	FTransform SpawnTransform;
};

USTRUCT()
struct FSpawningWorldActorsEntry
{
	GENERATED_BODY()

	// The world to spawn the actors for (can be left blank, in which case we'll spawn them for all worlds)
	//UPROPERTY(EditAnywhere, Category="Feature Data")
	//TSoftObjectPtr<UWorld> TargetWorld;

	// The actors to spawn
	UPROPERTY(EditAnywhere, Category = "Feature Data")
	TArray<FSpawningActorEntry> Actors;
};	


UCLASS(MinimalAPI, meta = (DisplayName = "Add Spawned Actors"))
class UGameFeatureAction_AddSpawnObject : public UGameFeatureAction_WorldActionBase
{
	GENERATED_BODY()
public:
	//~ Begin UGameFeatureAction interface
	virtual void OnGameFeatureActivating(FGameFeatureActivatingContext& Context) override;
	virtual void OnGameFeatureDeactivating(FGameFeatureDeactivatingContext& Context) override;
	//~ End UGameFeatureAction interface


	UPROPERTY(EditAnywhere, Category="Actor")
	TArray<FSpawningWorldActorsEntry> ActorsList;


private:

	//~ Begin UGameFeatureAction_WorldActionBase interface
	virtual void AddToWorld(const FWorldContext& WorldContext, const FGameFeatureStateChangeContext& ChangeContext) override;
	//~ End UGameFeatureAction_WorldActionBase interface

	void Reset();

	TArray<TWeakObjectPtr<AActor>> SpawnedActors;

};
