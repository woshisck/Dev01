// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "YogAnimNotify.generated.h"

/**
 * 
 */
UCLASS()
class DEVKIT_API UYogAnimNotify : public UAnimNotify
{
	GENERATED_BODY()

public:
	
	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Index", meta = (ExposeOnSpawn = "true", OverrideNativeName = "Index"))
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int Order_Index;


	UFUNCTION(BlueprintCallable)
	void CreateDmgTriangle();
	
	UFUNCTION(BlueprintCallable)
	void CreateDetectBox();

	UFUNCTION(BlueprintCallable)
	void SetOrderIndex(int index);

};
