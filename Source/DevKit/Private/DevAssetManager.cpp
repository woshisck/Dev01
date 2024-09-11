// Fill out your copyright notice in the Description page of Project Settings.


#include "DevAssetManager.h"

UDevAssetManager* UDevAssetManager::GetDevAssetManager() {
	UDevAssetManager* This = Cast<UDevAssetManager>(GEngine->AssetManager);

	if (This)
	{
		return This;
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Not Find AssetManager"));
		return NewObject<UDevAssetManager>();
	}
}

FString UDevAssetManager::AsyncLoadMap(FSoftObjectPath Path, FOnPackageLoaded OnPackageLoaded) {
	FString result;
	result += FString::Printf(TEXT("StartLoad:\t%s\n"), *Path.ToString());

	CurrentLoadPackage = Path.ToString();
	LoadPackageAsync(
		CurrentLoadPackage,
		FLoadPackageAsyncDelegate::CreateLambda([=](const FName& PackageName, UPackage* LoadedPackage, EAsyncLoadingResult::Type Result)
			{
				if (Result == EAsyncLoadingResult::Succeeded)
				{
					//可执行通知进行地图切换，即openlevel
					OnPackageLoaded.ExecuteIfBound();
				}
			}), 0, PKG_ContainsMap);
	return result;
}

float UDevAssetManager::GetCurrentLoadProgress(int32& LoadedCount, int32& RequestedCount) const {
	return GetAsyncLoadPercentage(*CurrentLoadPackage);
}