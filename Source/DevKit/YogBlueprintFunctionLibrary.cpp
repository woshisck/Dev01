// Fill out your copyright notice in the Description page of Project Settings.


#include "YogBlueprintFunctionLibrary.h"

UYogBlueprintFunctionLibrary::UYogBlueprintFunctionLibrary(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

bool UYogBlueprintFunctionLibrary::IsInEditor()
{
	return GIsEditor;
}

FName UYogBlueprintFunctionLibrary::GetDTRow(FString AssetName, int32 rowNum)
{
	FString result;
	result.Append(AssetName);
	result.Append(TEXT("Lvl_"));
	result += FString::Printf(TEXT("%u"), rowNum);

	return FName(result);
}
