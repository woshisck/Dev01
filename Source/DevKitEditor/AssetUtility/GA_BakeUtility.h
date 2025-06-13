// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AssetActionUtility.h"
#include "GA_BakeUtility.generated.h"

/**
 * 
 */
UCLASS()
class DEVKITEDITOR_API UGA_BakeUtility : public UAssetActionUtility
{
	GENERATED_BODY()
	

public:
	UGA_BakeUtility();
	
	// This makes the function appear in the right-click menu for selected assets
	UFUNCTION(CallInEditor, Category = "Asset Tools")
	void BakeData();

};
