// Copyright 2021 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFeatureAction.h"
#include "Animation/AnimInstance.h"
#include "Runtime/Launch/Resources/Version.h"
#include "GSCGameFeatureAction_AddAnimLayers.generated.h"

class UActorComponent;
class UGameInstance;
struct FComponentRequestHandle;

USTRUCT()
struct FGSCAnimLayerEntry
{
	GENERATED_BODY()

	// The base actor class to add anim layers to
	UPROPERTY(EditAnywhere, Category="Anim Layers")
	TSoftClassPtr<AActor> ActorClass;

	UPROPERTY(EditAnywhere, Category="Anim Layers")
	TArray<TSoftClassPtr<UAnimInstance>> AnimLayers;
};

/**
* GameFeatureAction responsible for "pushing" linked Anim Layers to main Animation Blueprint
*/
UCLASS(MinimalAPI, meta = (DisplayName = "Add Anim Layers (GAS Companion)"))
class UGSCGameFeatureAction_AddAnimLayers : public UGameFeatureAction
{
	GENERATED_BODY()
public:
	/** List of components to add to gameplay actors when this game feature is enabled */
	UPROPERTY(EditAnywhere, Category="Components", meta=(TitleProperty="ActorClass")) // ShowOnlyInnerProperties ?
	TArray<FGSCAnimLayerEntry> AnimLayerEntries;

	//~ Begin UGameFeatureAction interface
	virtual void OnGameFeatureActivating() override;
	virtual void OnGameFeatureDeactivating(FGameFeatureDeactivatingContext& Context) override;
#if WITH_EDITORONLY_DATA
	virtual void AddAdditionalAssetBundleData(FAssetBundleData& AssetBundleData) override;
#endif
	//~ End UGameFeatureAction interface

	//~ Begin UObject interface
#if WITH_EDITOR
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3
	virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#else
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) override;
#endif
#endif
	//~ End UObject interface

	template<class ComponentType>
	ComponentType* FindOrAddComponentForActor(AActor* Actor, const FGSCAnimLayerEntry& AnimLayerEntry)
	{
		return Cast<ComponentType>(FindOrAddComponentForActor(ComponentType::StaticClass(), Actor, AnimLayerEntry));
	}
	UActorComponent* FindOrAddComponentForActor(UClass* ComponentType, AActor* Actor, const FGSCAnimLayerEntry& AnimLayerEntry);

private:
	struct FActorExtensions
	{
		TArray<TSubclassOf<UAnimInstance>> AnimLayers;
	};

	FDelegateHandle GameInstanceStartHandle;

	TArray<TSharedPtr<FComponentRequestHandle>> ComponentRequestHandles;

	// ReSharper disable once CppUE4ProbableMemoryIssuesWithUObjectsInContainer
	TMap<AActor*, FActorExtensions> ActiveExtensions;

	void Reset();
	void HandleActorExtension(AActor* Actor, FName EventName, int32 EntryIndex);
	void AddAnimLayers(AActor* Actor, const FGSCAnimLayerEntry& Entry);
	void RemoveAnimLayers(AActor* Actor);

	void AddToWorld(const FWorldContext& WorldContext);
	void HandleGameInstanceStart(UGameInstance* GameInstance);
};
