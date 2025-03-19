// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "YogBlueprintFunctionLibrary.generated.h"

/**
 * 
 */
UCLASS()
class UYogBlueprintFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_UCLASS_BODY()
public:
	UFUNCTION(BlueprintPure, Category = Loading)
	static bool IsInEditor();


	UFUNCTION(BlueprintPure, Category = Loading)
	static FName GetDTRow(FString AssetName, int32 rowNum);


};

