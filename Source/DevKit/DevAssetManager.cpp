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