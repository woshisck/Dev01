// Fill out your copyright notice in the Description page of Project Settings.


#include "DevAssetManager.h"
#include "AbilitySystemGlobals.h"


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

void UDevAssetManager::AsyncLoadMap(FSoftObjectPath Path, FOnAsyncLoadFinished OnPackageLoaded) {
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

	UAbilitySystemGlobals::Get().InitGlobalData();
}