#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardData.h"
#include "Data/EnemyData.h"

namespace
{
template <typename T>
T* LoadTestAsset(const TCHAR* ObjectPath)
{
	return Cast<T>(StaticLoadObject(T::StaticClass(), nullptr, ObjectPath));
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEnemyAIDefaultTemplateTest,
	"DevKit.EnemyAI.DefaultTemplate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEnemyAIDefaultTemplateTest::RunTest(const FString& Parameters)
{
	UBlackboardData* Blackboard = LoadTestAsset<UBlackboardData>(TEXT("/Game/Code/Enemy/AI/BlackBoard/BB_Enemy_DefaultMelee.BB_Enemy_DefaultMelee"));
	UBehaviorTree* BehaviorTree = LoadTestAsset<UBehaviorTree>(TEXT("/Game/Code/Enemy/AI/Behaviour/BT_Enemy_DefaultMelee.BT_Enemy_DefaultMelee"));
	UEnemyData* RatData = LoadTestAsset<UEnemyData>(TEXT("/Game/Docs/Data/Enemy/Rat/DA_Rat.DA_Rat"));
	UEnemyData* RottenGuardData = LoadTestAsset<UEnemyData>(TEXT("/Game/Docs/Data/Enemy/RottenGuard/DA_RottenGuard.DA_RottenGuard"));

	TestNotNull(TEXT("Default melee blackboard exists"), Blackboard);
	TestNotNull(TEXT("Default melee behavior tree exists"), BehaviorTree);
	TestNotNull(TEXT("DA_Rat exists"), RatData);
	TestNotNull(TEXT("DA_RottenGuard exists"), RottenGuardData);
	if (!Blackboard || !BehaviorTree || !RatData || !RottenGuardData)
	{
		return false;
	}

	static const FName RequiredKeys[] = {
		TEXT("EnemyAIState"),
		TEXT("TargetActor"),
		TEXT("LastKnownTargetLocation"),
		TEXT("PatrolOriginLocation"),
		TEXT("PatrolTargetLocation"),
		TEXT("MoveTargetLocation"),
		TEXT("DistanceToTarget"),
		TEXT("bInAttackRange"),
		TEXT("AcceptanceRadius"),
		TEXT("AlertExpireTime"),
		TEXT("LastSeenTargetTime"),
	};

	for (const FName KeyName : RequiredKeys)
	{
		TestTrue(FString::Printf(TEXT("Blackboard contains key %s"), *KeyName.ToString()), Blackboard->GetKeyID(KeyName) != FBlackboard::InvalidKey);
	}

	TestEqual(TEXT("Behavior tree uses generated blackboard"), BehaviorTree->BlackboardAsset.Get(), Blackboard);
	TestNotNull(TEXT("Behavior tree has root node"), BehaviorTree->RootNode.Get());
	TestEqual(TEXT("DA_Rat uses default melee behavior tree"), RatData->BehaviorTree.Get(), BehaviorTree);
	TestEqual(TEXT("DA_RottenGuard uses default melee behavior tree"), RottenGuardData->BehaviorTree.Get(), BehaviorTree);

	return true;
}

#endif
