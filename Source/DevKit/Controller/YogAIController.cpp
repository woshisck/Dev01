// Fill out your copyright notice in the Description page of Project Settings.


#include "YogAIController.h"

#include "Kismet/GameplayStatics.h"
#include "Navigation/CrowdFollowingComponent.h"
#include "DevKit/Enemy/EnemyCharacterBase.h"


AYogAIController::AYogAIController()
{
    BehaviorTreeComponent = CreateDefaultSubobject<UBehaviorTreeComponent>(TEXT("BehaviorTreeComponent"));
    BlackboardComponent = CreateDefaultSubobject<UBlackboardComponent>(TEXT("BlackboardComponent"));
}

void AYogAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

    // Get a reference to the possessed Character

    //AEnemyCharacterBase* AICharacter = Cast<AEnemyCharacterBase>(InPawn);

    //check(AICharacter)
    

    //TODO: AI BB BT
    //if (AICharacter && AICharacter->BehaviorTree)
    //{
    //    // Initialize the Blackboard using the Asset from the BehaviorTree
    //    if (AICharacter->BehaviorTree->BlackboardAsset)
    //    {
    //        BlackboardComponent->InitializeBlackboard(*(AICharacter->BehaviorTree->BlackboardAsset));
    //        // Store key IDs for later use
    //        TargetKeyID = BlackboardComponent->GetKeyID("TargetActor");
    //    }
    //    // Start the Behavior Tree
    //    BehaviorTreeComponent->StartTree(*(AICharacter->BehaviorTree));
    //}
}

void AYogAIController::InitializeDefaultAI()
{
    UBlackboardData* BBAsset = Default_BehaviourTree->BlackboardAsset;
    if (BBAsset && BlackboardComponent->InitializeBlackboard(*BBAsset))
    {
        // Optionally, set an initial value for a Blackboard key here.
        // For example, to set the initial 'TargetActor', you could do:
        // BlackboardComponent->SetValueAsObject(TargetActorKeyName, SomeActorObject);
        // Start the Behavior Tree
        BehaviorTreeComponent->StartTree(*Default_BehaviourTree);
    }
}
