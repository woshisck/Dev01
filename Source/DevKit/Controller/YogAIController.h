// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ModularAIController.h"
#include "YogAIController.generated.h"

/**
 * 
 */
UCLASS()
class DEVKIT_API AYogAIController : public AModularAIController
{
	GENERATED_BODY()
	
public:
	AYogAIController(const FObjectInitializer& ObjectInitializer);

	virtual void OnPossess(APawn* InPawn) override;
};
