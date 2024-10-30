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