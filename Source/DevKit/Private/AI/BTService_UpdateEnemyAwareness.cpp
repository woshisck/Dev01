#include "AI/BTService_UpdateEnemyAwareness.h"

#include "BehaviorTree/Blackboard/BlackboardKeyType_Enum.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Float.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Controller/YogAIController.h"

UBTService_UpdateEnemyAwareness::UBTService_UpdateEnemyAwareness()
{
	NodeName = TEXT("Update Enemy Awareness");
	bNotifyTick = true;
	Interval = 0.2f;
	RandomDeviation = 0.05f;

	EnemyAIStateKey.SelectedKeyName = TEXT("EnemyAIState");
	TargetActorKey.SelectedKeyName = TEXT("TargetActor");
	LastKnownTargetLocationKey.SelectedKeyName = TEXT("LastKnownTargetLocation");
	AlertExpireTimeKey.SelectedKeyName = TEXT("AlertExpireTime");
	LastSeenTargetTimeKey.SelectedKeyName = TEXT("LastSeenTargetTime");

	EnemyAIStateKey.AddEnumFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_UpdateEnemyAwareness, EnemyAIStateKey), StaticEnum<EEnemyAIState>());
	TargetActorKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_UpdateEnemyAwareness, TargetActorKey), AActor::StaticClass());
	LastKnownTargetLocationKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_UpdateEnemyAwareness, LastKnownTargetLocationKey));
	AlertExpireTimeKey.AddFloatFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_UpdateEnemyAwareness, AlertExpireTimeKey));
	LastSeenTargetTimeKey.AddFloatFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_UpdateEnemyAwareness, LastSeenTargetTimeKey));
}

void UBTService_UpdateEnemyAwareness::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	AYogAIController* YogAI = Cast<AYogAIController>(OwnerComp.GetAIOwner());
	if (!YogAI)
	{
		return;
	}

	YogAI->UpdateAwarenessBlackboard(
		OwnerComp.GetBlackboardComponent(),
		EnemyAIStateKey.SelectedKeyName,
		TargetActorKey.SelectedKeyName,
		LastKnownTargetLocationKey.SelectedKeyName,
		AlertExpireTimeKey.SelectedKeyName,
		LastSeenTargetTimeKey.SelectedKeyName);
}
