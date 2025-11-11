// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ModularAIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "YogAIController.generated.h"

/**
 * 
 */

UCLASS()
class DEVKIT_API AYogAIController : public AModularAIController
{
	GENERATED_BODY()
	
public:
	AYogAIController();

	virtual void OnPossess(APawn* InPawn) override;


	// The component that will run the Behavior Tree
	UPROPERTY(BlueprintReadWrite, Category = "AI")
	UBehaviorTreeComponent* BehaviorTreeComponent;

	// The component that holds the Blackboard data
	UPROPERTY(BlueprintReadWrite, Category = "AI")
	UBlackboardComponent* BlackboardComponent;


};
