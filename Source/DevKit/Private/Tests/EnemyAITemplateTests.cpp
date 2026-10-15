#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "BehaviorTree/BlackboardData.h"
#include "Data/AbilityData.h"
#include "Data/EnemyData.h"
#include "StateTree.h"

template <typename T>
static T* LoadTestAsset(const TCHAR* ObjectPath)
{
	return Cast<T>(StaticLoadObject(T::StaticClass(), nullptr, ObjectPath));
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEnemyAIDefaultTemplateTest,
	"DevKit.EnemyAI.DefaultTemplate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEnemyAIDefaultTemplateTest::RunTest(const FString& Parameters)
{
	UBlackboardData* Blackboard = LoadTestAsset<UBlackboardData>(TEXT("/Game/Code/Enemy/AI/BlackBoard/BB_Enemy_DefaultMelee.BB_Enemy_DefaultMelee"));
	UStateTree* StateTree = LoadTestAsset<UStateTree>(TEXT("/Game/Code/Enemy/AI/StateTree/ST_Enemy_DefaultMelee.ST_Enemy_DefaultMelee"));
	UEnemyData* RatData = LoadTestAsset<UEnemyData>(TEXT("/Game/Docs/Data/Enemy/Rat/DA_Rat.DA_Rat"));
	UEnemyData* RottenGuardData = LoadTestAsset<UEnemyData>(TEXT("/Game/Docs/Data/Enemy/RottenGuard/DA_RottenGuard.DA_RottenGuard"));

	TestNotNull(TEXT("Default melee blackboard exists"), Blackboard);
	TestNotNull(TEXT("Default melee state tree exists"), StateTree);
	TestNotNull(TEXT("DA_Rat exists"), RatData);
	TestNotNull(TEXT("DA_RottenGuard exists"), RottenGuardData);
	if (!Blackboard || !StateTree || !RatData || !RottenGuardData)
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
		TEXT("bLastAttackWhiffed"),
		TEXT("LastWhiffTime"),
		TEXT("LastAttackResolveTime"),
		TEXT("bPostAttackReposition"),
		TEXT("LastRepositionRequestTime"),
	};

	for (const FName KeyName : RequiredKeys)
	{
		TestTrue(FString::Printf(TEXT("Blackboard contains key %s"), *KeyName.ToString()), Blackboard->GetKeyID(KeyName) != FBlackboard::InvalidKey);
	}

	TestEqual(TEXT("DA_Rat uses default melee state tree"), RatData->StateTree.Get(), StateTree);
	TestEqual(TEXT("DA_RottenGuard uses default melee state tree"), RottenGuardData->StateTree.Get(), StateTree);
	TestEqual(TEXT("DA_Rat uses default melee state tree blackboard"), RatData->StateTreeBlackboard.Get(), Blackboard);
	TestEqual(TEXT("DA_RottenGuard uses default melee state tree blackboard"), RottenGuardData->StateTreeBlackboard.Get(), Blackboard);
	TestEqual(TEXT("DA_Rat starts with swarm flank movement"), RatData->MovementTuning.ApproachStyle, EEnemyAIApproachStyle::SwarmFlank);
	TestEqual(TEXT("DA_Rat attack range is configured"), RatData->MovementTuning.AttackRange, 150.0f);
	TestEqual(TEXT("DA_Rat locks combat slot briefly"), RatData->MovementTuning.CombatSlotLockDuration, 1.2f);
	TestEqual(TEXT("DA_Rat has attack range hysteresis"), RatData->MovementTuning.AttackRangeExitBuffer, 40.0f);
	TestEqual(TEXT("DA_Rat has sticky combat exit radius"), RatData->AwarenessTuning.CombatExitRadius, 1200.0f);
	TestTrue(TEXT("DA_Rat uses forward steering"), RatData->MovementTuning.bUseForwardSteering);
	TestEqual(TEXT("DA_RottenGuard starts with bruiser movement"), RottenGuardData->MovementTuning.ApproachStyle, EEnemyAIApproachStyle::BruiserHold);
	TestEqual(TEXT("DA_RottenGuard attack range is configured"), RottenGuardData->MovementTuning.AttackRange, 260.0f);
	TestTrue(TEXT("DA_RottenGuard uses forward steering"), RottenGuardData->MovementTuning.bUseForwardSteering);
	TestEqual(TEXT("DA_RottenGuard has speed override"), RottenGuardData->MovementTuning.MaxWalkSpeedOverride, 420.0f);
	TestEqual(TEXT("DA_RottenGuard has sticky combat exit radius"), RottenGuardData->AwarenessTuning.CombatExitRadius, 1200.0f);

	// Attack definitions live on the StateTree states now, so the DataAsset contract is only
	// that the ability tags those states fire are actually granted here.
	static const TCHAR* RequiredAbilityTagNames[] = {
		TEXT("Enemy.Melee.LAtk1"),
		TEXT("Enemy.Melee.HAtk1"),
		TEXT("Enemy.Melee.HAtk2"),
	};

	TestNotNull(TEXT("DA_Rat has AbilityData"), RatData->AbilityData.Get());
	TestNotNull(TEXT("DA_RottenGuard has AbilityData"), RottenGuardData->AbilityData.Get());

	for (const TCHAR* TagName : RequiredAbilityTagNames)
	{
		const FGameplayTag AbilityTag = FGameplayTag::RequestGameplayTag(TagName, false);
		TestTrue(FString::Printf(TEXT("%s tag is registered"), TagName), AbilityTag.IsValid());
	}

	const FGameplayTag RottenGuardLightAttackTag = FGameplayTag::RequestGameplayTag(TEXT("Enemy.Melee.LAtk1"), false);
	if (RottenGuardLightAttackTag.IsValid() && RottenGuardData->AbilityData)
	{
		TestTrue(TEXT("DA_RottenGuard AbilityData has valid LAtk1 montage or montage config"), RottenGuardData->AbilityData->HasAbility(RottenGuardLightAttackTag));
	}

	return true;
}

#endif
