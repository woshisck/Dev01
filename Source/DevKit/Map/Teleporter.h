// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Teleporter.generated.h"

/**
 * 
 */
UCLASS(Blueprintable, BlueprintType)
class DEVKIT_API ATeleporter : public AActor
{
	GENERATED_BODY()
public:


	ATeleporter();
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level reference")
	TArray<TSoftObjectPtr<UWorld>> MapBase;

};
