// Fill out your copyright notice in the Description page of Project Settings.


#include "YogAIController.h"

#include "Kismet/GameplayStatics.h"
#include "Navigation/CrowdFollowingComponent.h"
#include "DevKit/Enemy/EnemyCharacterBase.h"


AYogAIController::AYogAIController()
{
    BehaviorTreeComponent = CreateDefaultSubobject<UBehaviorTreeComponent>(TEXT("YogBT"));
    BlackboardComponent = CreateDefaultSubobject<UBlackboardComponent>(TEXT("YogBB"));
}

void AYogAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);



    // Get a reference to the possessed Character

    //AEnemyCharacterBase* AICharacter = Cast<AEnemyCharacterBase>(InPawn);

    //ensure(AICharacter && AICharacter->CharacterData);
    //UEnemyData* enemy_data = Cast<UEnemyData>(AICharacter->CharacterData);

    //if (IsValid(enemy_data->EnemyBT))
    //{
    //    //// Initialize the Blackboard using the Blackboard asset from the Behavior Tree
    //    //// This also creates the BlackboardComponent if it doesn't exist
    //    //UseBlackboard(BehaviorTree->BlackboardAsset, BlackboardComponent);

    //    // Start running the Behavior Tree
    //    RunBehaviorTree(enemy_data->EnemyBT);
    //}

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

