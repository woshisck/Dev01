// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/StreamableManager.h"
#include "Engine/AssetManager.h"
#include "Player/PlayerCharacterBase.h"
#include "DevAssetManager.generated.h"


DECLARE_DYNAMIC_DELEGATE(FOnAsyncLoadFinished);

UCLASS()
class DEVKIT_API UDevAssetManager : public UAssetManager
{
	GENERATED_BODY()
	

public:

	UDevAssetManager();

	bool bIsLoaded;
	
	FString CurrentLoadPackage;


	virtual void StartInitialLoading() override;

	UFUNCTION(BlueprintPure,BlueprintCallable, Category = "AssetLoader")
	static UDevAssetManager* GetDevAssetManager();

	// Returns the asset referenced by a TSoftObjectPtr.  This will synchronously load the asset if it's not already loaded.
	template<typename AssetType>
	static AssetType* GetAsset(const TSoftObjectPtr<AssetType>& AssetPointer, bool bKeepInMemory = true);

	// Returns the subclass referenced by a TSoftClassPtr.  This will synchronously load the asset if it's not already loaded.
	template<typename AssetType>
	static TSubclassOf<AssetType> GetSubclass(const TSoftClassPtr<AssetType>& AssetPointer, bool bKeepInMemory = true);


	FOnAsyncLoadFinished OnLoadFinished;
	FStreamableManager AssetLoader;

	//UFUNCTION(BlueprintCallable, CallInEditor)
	//FString AsyncLoadAsset(const FOnAsyncLoadFinished& OnAsyncLoadFinished);
	UFUNCTION(BlueprintCallable)
	void AsyncLoadAsset(FSoftObjectPath Path, FOnAsyncLoadFinished OnAsyncLoadFinished);


	UFUNCTION(BlueprintCallable, CallInEditor)
	float GetLoadProgress();
	//float GetCurrentLoadProgress(int32& LoadedCount, int32& RequestedCount) const;
	

	// Logs all assets currently loaded and tracked by the asset manager.
	static void DumpLoadedAssets();

protected:
	// Thread safe way of adding a loaded asset to keep in memory.
	void AddLoadedAsset(const UObject* Asset);

	// Loaded version of the game data
	UPROPERTY(Transient)
	TMap<TObjectPtr<UClass>, TObjectPtr<UPrimaryDataAsset>> GameDataMap;


private:

	// Assets loaded and tracked by the asset manager.
	UPROPERTY()
	TSet<TObjectPtr<const UObject>> LoadedAssets;

	FCriticalSection LoadedAssetsCritical;



};

template<typename AssetType>
AssetType* UDevAssetManager::GetAsset(const TSoftObjectPtr<AssetType>& AssetPointer, bool bKeepInMemory)
{
	AssetType* LoadedAsset = nullptr;

	const FSoftObjectPath& AssetPath = AssetPointer.ToSoftObjectPath();

	if (AssetPath.IsValid())
	{
		LoadedAsset = AssetPointer.Get();
		if (!LoadedAsset)
		{
			LoadedAsset = Cast<AssetType>(SynchronousLoadAsset(AssetPath));
			ensureAlwaysMsgf(LoadedAsset, TEXT("Failed to load asset [%s]"), *AssetPointer.ToString());
		}

		if (LoadedAsset && bKeepInMemory)
		{
			// Added to loaded asset list.
			Get().AddLoadedAsset(Cast<UObject>(LoadedAsset));
		}
	}

	return LoadedAsset;
}

template<typename AssetType>
TSubclassOf<AssetType> UDevAssetManager::GetSubclass(const TSoftClassPtr<AssetType>& AssetPointer, bool bKeepInMemory)
{
	TSubclassOf<AssetType> LoadedSubclass;

	const FSoftObjectPath& AssetPath = AssetPointer.ToSoftObjectPath();

	if (AssetPath.IsValid())
	{
		LoadedSubclass = AssetPointer.Get();
		if (!LoadedSubclass)
		{
			LoadedSubclass = Cast<UClass>(SynchronousLoadAsset(AssetPath));
			ensureAlwaysMsgf(LoadedSubclass, TEXT("Failed to load asset class [%s]"), *AssetPointer.ToString());
		}

		if (LoadedSubclass && bKeepInMemory)
		{
			// Added to loaded asset list.
			Get().AddLoadedAsset(Cast<UObject>(LoadedSubclass));
		}
	}

	return LoadedSubclass;
}
