// Fill out your copyright notice in the Description page of Project Settings.


#include "YogAIController.h"


#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Navigation/CrowdFollowingComponent.h"
#include "DevKit/Enemy/EnemyCharacterBase.h"


AYogAIController::AYogAIController(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer.SetDefaultSubobjectClass<UCrowdFollowingComponent>(TEXT("PathFollowingComponent")))
{

}

void AYogAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

    // Get a reference to the possessed Character
    AEnemyCharacterBase* AICharacter = Cast<AEnemyCharacterBase>(InPawn);

    check(AICharacter)
    

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
