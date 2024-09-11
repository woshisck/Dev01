// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/AssetManager.h"
#include "DevAssetManager.generated.h"


DECLARE_DYNAMIC_DELEGATE(FOnPackageLoaded);

UCLASS()
class DEVKIT_API UDevAssetManager : public UAssetManager
{
	GENERATED_BODY()
	

public:
	FString CurrentLoadPackage;

	UFUNCTION(BlueprintPure,BlueprintCallable, Category = "AssetLoader")
	static UDevAssetManager* GetDevAssetManager();

	UFUNCTION(BlueprintCallable, CallInEditor)
	FString AsyncLoadMap(FSoftObjectPath Path, FOnPackageLoaded OnPackageLoaded);
	
	UFUNCTION(BlueprintCallable, CallInEditor)
	float GetCurrentLoadProgress(int32& LoadedCount, int32& RequestedCount) const;
	
};
