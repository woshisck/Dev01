// Fill out your copyright notice in the Description page of Project Settings.


#include "DevAssetManager.h"
#include "AbilitySystemGlobals.h"
#include "DevKit/Data/YogGameData.h"

UDevAssetManager::UDevAssetManager()
{

}



UDevAssetManager* UDevAssetManager::GetDevAssetManager() {
	
	check(GEngine);

	if (UDevAssetManager* Singleton = Cast<UDevAssetManager>(GEngine->AssetManager))
	{
		return Singleton;
	}

	UE_LOG(LogTemp, Fatal, TEXT("Invalid AssetManagerClassName in DefaultEngine.ini.  It must be set to LyraAssetManager!"));

	return NewObject<UDevAssetManager>();

}

const UYogGameData& UDevAssetManager::GetGameData()
{
	return GetOrLoadTypedGameData<UYogGameData>(YogGameDataPath);
}

void UDevAssetManager::AsyncLoadAsset(FSoftObjectPath Path, FOnAsyncLoadFinished OnPackageLoaded) {
	FString result;
	result += FString::Printf(TEXT("StartLoad:\t%s\n"), *Path.ToString());
	bIsLoaded = false;

	CurrentLoadPackage = Path.ToString();
	LoadPackageAsync(
		CurrentLoadPackage,
		FLoadPackageAsyncDelegate::CreateLambda([=,this](const FName& PackageName, UPackage* LoadedPackage, EAsyncLoadingResult::Type Result)
			{
				if (Result == EAsyncLoadingResult::Succeeded)
				{
					bIsLoaded = true;
					UE_LOG(LogTemp, Warning, TEXT("Load Succeeded"));
					OnPackageLoaded.ExecuteIfBound();
				}
				else if(Result == EAsyncLoadingResult::Failed){
					UE_LOG(LogTemp, Warning, TEXT("Load Failed"));
				}

			}), 0, PKG_ContainsMap);
}


float UDevAssetManager::GetLoadProgress() {
	float FloatPercentage = GetAsyncLoadPercentage(*CurrentLoadPackage);
	if (!bIsLoaded) {
		FString ResutlStr = FString::Printf(TEXT("Percentage: %f"), FloatPercentage);
		GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Green, ResutlStr);
		UE_LOG(LogTemp, Warning, TEXT("Percentage: %f"), FloatPercentage);
	}
	else {
		FloatPercentage = 100;

	}
	return FloatPercentage;
}

void UDevAssetManager::DumpLoadedAssets()
{
	UE_LOG(LogTemp, Log, TEXT("========== Start Dumping Loaded Assets =========="));

	for (const UObject* LoadedAsset : GetDevAssetManager()->LoadedAssets)
	{
		UE_LOG(LogTemp, Log, TEXT("  %s"), *GetNameSafe(LoadedAsset));
	}

	UE_LOG(LogTemp, Log, TEXT("... %d assets in loaded pool"), GetDevAssetManager()->LoadedAssets.Num());
	UE_LOG(LogTemp, Log, TEXT("========== Finish Dumping Loaded Assets =========="));
}

UObject* UDevAssetManager::SynchronousLoadAsset(const FSoftObjectPath& AssetPath)
{
	if (AssetPath.IsValid())
	{
		TUniquePtr<FScopeLogTime> LogTimePtr;

		if (ShouldLogAssetLoads())
		{
			LogTimePtr = MakeUnique<FScopeLogTime>(*FString::Printf(TEXT("Synchronously loaded asset [%s]"), *AssetPath.ToString()), nullptr, FScopeLogTime::ScopeLog_Seconds);
		}

		if (UAssetManager::IsInitialized())
		{
			return UAssetManager::GetStreamableManager().LoadSynchronous(AssetPath, false);
		}

		// Use LoadObject if asset manager isn't ready yet.
		return AssetPath.TryLoad();
	}

	return nullptr;
}

bool UDevAssetManager::ShouldLogAssetLoads()
{
	static bool bLogAssetLoads = FParse::Param(FCommandLine::Get(), TEXT("LogAssetLoads"));
	return bLogAssetLoads;
}

void UDevAssetManager::AddLoadedAsset(const UObject* Asset)
{
	if (ensureAlways(Asset))
	{
		FScopeLock LoadedAssetsLock(&LoadedAssetsCritical);
		LoadedAssets.Add(Asset);
	}
}


void UDevAssetManager::StartInitialLoading()
{
	Super::StartInitialLoading();

	SCOPED_BOOT_TIMING("UDevAssetManager::StartInitialLoading");

	// This does all of the scanning, need to do this now even if loads are deferred
	Super::StartInitialLoading();

	//STARTUP_JOB(InitializeGameplayCueManager());

	//{
	//	// Load base game data asset
	//	STARTUP_JOB_WEIGHTED(GetGameData(), 25.f);
	//}

	//// Run all the queued up startup jobs
	//DoAllStartupJobs();

	UAbilitySystemGlobals::Get().InitGlobalData();

}



UPrimaryDataAsset* UDevAssetManager::LoadGameDataOfClass(TSubclassOf<UPrimaryDataAsset> DataClass, const TSoftObjectPtr<UPrimaryDataAsset>& DataClassPath, FPrimaryAssetType PrimaryAssetType)
{
	UPrimaryDataAsset* Asset = nullptr;

	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("Loading GameData Object"), STAT_GameData, STATGROUP_LoadTime);
	if (!DataClassPath.IsNull())
	{
#if WITH_EDITOR
		FScopedSlowTask SlowTask(0, FText::Format(NSLOCTEXT("LyraEditor", "BeginLoadingGameDataTask", "Loading GameData {0}"), FText::FromName(DataClass->GetFName())));
		const bool bShowCancelButton = false;
		const bool bAllowInPIE = true;
		SlowTask.MakeDialog(bShowCancelButton, bAllowInPIE);
#endif
		UE_LOG(LogTemp, Log, TEXT("Loading GameData: %s ..."), *DataClassPath.ToString());
		SCOPE_LOG_TIME_IN_SECONDS(TEXT("    ... GameData loaded!"), nullptr);

		// This can be called recursively in the editor because it is called on demand from PostLoad so force a sync load for primary asset and async load the rest in that case
		if (GIsEditor)
		{
			Asset = DataClassPath.LoadSynchronous();
			LoadPrimaryAssetsWithType(PrimaryAssetType);
		}
		else
		{
			TSharedPtr<FStreamableHandle> Handle = LoadPrimaryAssetsWithType(PrimaryAssetType);
			if (Handle.IsValid())
			{
				Handle->WaitUntilComplete(0.0f, false);

				// This should always work
				Asset = Cast<UPrimaryDataAsset>(Handle->GetLoadedAsset());
			}
		}
	}

	if (Asset)
	{
		GameDataMap.Add(DataClass, Asset);
	}
	else
	{
		// It is not acceptable to fail to load any GameData asset. It will result in soft failures that are hard to diagnose.
		UE_LOG(LogTemp, Fatal, TEXT("Failed to load GameData asset at %s. Type %s. This is not recoverable and likely means you do not have the correct data to run %s."), *DataClassPath.ToString(), *PrimaryAssetType.ToString(), FApp::GetProjectName());
	}

	return Asset;
}


#if WITH_EDITOR
void UDevAssetManager::PreBeginPIE(bool bStartSimulate)
{
	Super::PreBeginPIE(bStartSimulate);
	{
		FScopedSlowTask SlowTask(0, NSLOCTEXT("LyraEditor", "BeginLoadingPIEData", "Loading PIE Data"));
		const bool bShowCancelButton = false;
		const bool bAllowInPIE = true;
		SlowTask.MakeDialog(bShowCancelButton, bAllowInPIE);

		const UYogGameData& LocalGameDataCommon = GetGameData();

		// Intentionally after GetGameData to avoid counting GameData time in this timer
		SCOPE_LOG_TIME_IN_SECONDS(TEXT("PreBeginPIE asset preloading complete"), nullptr);

		// You could add preloading of anything else needed for the experience we'll be using here
		// (e.g., by grabbing the default experience from the world settings + the experience override in developer settings)
	}
}
#endif