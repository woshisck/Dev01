// Fill out your copyright notice in the Description page of Project Settings.


#include "Controller/YogAIController.h"

#include "Kismet/GameplayStatics.h"
#include "Navigation/CrowdFollowingComponent.h"
#include "Character/EnemyCharacterBase.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardData.h"


AYogAIController::AYogAIController()
{
    BehaviorTreeComponent = CreateDefaultSubobject<UBehaviorTreeComponent>(TEXT("YogBT"));
    BlackboardComponent = CreateDefaultSubobject<UBlackboardComponent>(TEXT("YogBB"));
}

bool AYogAIController::RunBTWithBlackboard(UBehaviorTree* BT, UBlackboardData* BB)
{
    if (!BT)
    {
        UE_LOG(LogTemp, Warning, TEXT("[YogAIController] RunBTWithBlackboard FAIL: BT is null"));
        return false;
    }
    // 1) 先绑黑板（绕开 BT 内部 BlackboardAsset 断链）
    if (BB && BlackboardComponent)
    {
        const bool bUsed = UseBlackboard(BB, BlackboardComponent);
        UE_LOG(LogTemp, Warning, TEXT("[YogAIController] UseBlackboard(%s) -> %d"),
            *BB->GetName(), bUsed ? 1 : 0);
    }
    // 2) 启动 BT（RunBehaviorTree 内部如果发现 BB 已绑会复用，不会覆盖）
    const bool bRan = RunBehaviorTree(BT);
    UE_LOG(LogTemp, Warning, TEXT("[YogAIController] RunBehaviorTree(%s) -> %d"),
        *BT->GetName(), bRan ? 1 : 0);
    return bRan;
}

void AYogAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	// 兜底启动：BT 内部 BlackboardAsset 断链时由 C++ 强制 UseBlackboard + RunBehaviorTree
	if (bUseFallbackStartup && FallbackBehaviorTree)
	{
		RunBTWithBlackboard(FallbackBehaviorTree, FallbackBlackboard);
	}



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

