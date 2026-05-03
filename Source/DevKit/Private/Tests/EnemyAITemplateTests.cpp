#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "AI/BTTask_EnemyAttackByProfile.h"
#include "AI/BTTask_EnemyCombatMove.h"
#include "BehaviorTree/BTCompositeNode.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardData.h"
#include "Data/AbilityData.h"
#include "Data/EnemyData.h"

namespace
{
template <typename T>
T* LoadTestAsset(const TCHAR* ObjectPath)
{
	return Cast<T>(StaticLoadObject(T::StaticClass(), nullptr, ObjectPath));
}

const FEnemyAIAttackOption* FindAttackByTag(const FEnemyAIAttackProfile& AttackProfile, const FGameplayTag& AbilityTag)
{
	return AttackProfile.Attacks.FindByPredicate(
		[AbilityTag](const FEnemyAIAttackOption& Attack)
		{
			return Attack.AbilityTags.HasTagExact(AbilityTag);
		});
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
	const UBTCompositeNode* MainSelector = BehaviorTree->RootNode.Get();
	const UBTCompositeNode* CombatSelector = nullptr;
	if (MainSelector)
	{
		TestTrue(TEXT("Behavior tree main selector has combat child"), MainSelector->Children.Num() >= 1);
		CombatSelector = MainSelector->Children.IsValidIndex(0) ? MainSelector->Children[0].ChildComposite : nullptr;
	}
	TestNotNull(TEXT("Combat branch is a selector child"), CombatSelector);
	if (CombatSelector)
	{
		TestTrue(TEXT("Combat branch has skill, special, close, and move children"), CombatSelector->Children.Num() >= 4);
		if (CombatSelector->Children.Num() >= 4)
		{
			const UBTTask_EnemyAttackByProfile* SkillTask = Cast<UBTTask_EnemyAttackByProfile>(CombatSelector->Children[0].ChildTask);
			const UBTTask_EnemyAttackByProfile* SpecialTask = Cast<UBTTask_EnemyAttackByProfile>(CombatSelector->Children[1].ChildTask);
			const UBTTask_EnemyAttackByProfile* CloseTask = Cast<UBTTask_EnemyAttackByProfile>(CombatSelector->Children[2].ChildTask);
			const UBTTask_EnemyCombatMove* MoveTask = Cast<UBTTask_EnemyCombatMove>(CombatSelector->Children[3].ChildTask);
			TestNotNull(TEXT("Combat child 0 is skill attack task"), SkillTask);
			TestNotNull(TEXT("Combat child 1 is special movement attack task"), SpecialTask);
			TestNotNull(TEXT("Combat child 2 is close melee attack task"), CloseTask);
			TestNotNull(TEXT("Combat child 3 is combat move task"), MoveTask);
			if (SkillTask)
			{
				TestEqual(TEXT("Combat child 0 role is Skill"), SkillTask->GetRequiredAttackRole(), EEnemyAIAttackRole::Skill);
			}
			if (SpecialTask)
			{
				TestEqual(TEXT("Combat child 1 role is SpecialMovement"), SpecialTask->GetRequiredAttackRole(), EEnemyAIAttackRole::SpecialMovement);
			}
			if (CloseTask)
			{
				TestEqual(TEXT("Combat child 2 role is CloseMelee"), CloseTask->GetRequiredAttackRole(), EEnemyAIAttackRole::CloseMelee);
			}
		}
	}
	TestEqual(TEXT("DA_Rat uses default melee behavior tree"), RatData->BehaviorTree.Get(), BehaviorTree);
	TestEqual(TEXT("DA_RottenGuard uses default melee behavior tree"), RottenGuardData->BehaviorTree.Get(), BehaviorTree);
	TestEqual(TEXT("DA_Rat starts with swarm flank movement"), RatData->MovementTuning.ApproachStyle, EEnemyAIApproachStyle::SwarmFlank);
	TestEqual(TEXT("DA_Rat attack range is configured"), RatData->MovementTuning.AttackRange, 150.0f);
	TestEqual(TEXT("DA_Rat locks combat slot briefly"), RatData->MovementTuning.CombatSlotLockDuration, 1.2f);
	TestEqual(TEXT("DA_Rat has attack range hysteresis"), RatData->MovementTuning.AttackRangeExitBuffer, 40.0f);
	TestEqual(TEXT("DA_Rat has sticky combat exit radius"), RatData->AwarenessTuning.CombatExitRadius, 1200.0f);
	TestTrue(TEXT("DA_Rat uses forward steering"), RatData->MovementTuning.bUseForwardSteering);
	TestTrue(TEXT("DA_Rat has default attacks"), RatData->AttackProfile.Attacks.Num() >= 1);
	for (const FEnemyAIAttackOption& RatAttack : RatData->AttackProfile.Attacks)
	{
		TestEqual(TEXT("DA_Rat attacks are close melee role"), RatAttack.AttackRole, EEnemyAIAttackRole::CloseMelee);
	}
	TestEqual(TEXT("DA_RottenGuard starts with bruiser movement"), RottenGuardData->MovementTuning.ApproachStyle, EEnemyAIApproachStyle::BruiserHold);
	TestEqual(TEXT("DA_RottenGuard attack range is configured"), RottenGuardData->MovementTuning.AttackRange, 260.0f);
	TestTrue(TEXT("DA_RottenGuard uses forward steering"), RottenGuardData->MovementTuning.bUseForwardSteering);
	TestEqual(TEXT("DA_RottenGuard has speed override"), RottenGuardData->MovementTuning.MaxWalkSpeedOverride, 420.0f);
	TestEqual(TEXT("DA_RottenGuard has sticky combat exit radius"), RottenGuardData->AwarenessTuning.CombatExitRadius, 1200.0f);
	TestTrue(TEXT("DA_RottenGuard has default attacks"), RottenGuardData->AttackProfile.Attacks.Num() >= 1);
	const FEnemyAIAttackOption* SweepAttack = RottenGuardData->AttackProfile.Attacks.FindByPredicate(
		[](const FEnemyAIAttackOption& Attack)
		{
			return Attack.AttackName == TEXT("Sweep");
		});
	TestNotNull(TEXT("DA_RottenGuard has Sweep attack"), SweepAttack);
	if (SweepAttack)
	{
		TestEqual(TEXT("DA_RottenGuard Sweep is close melee role"), SweepAttack->AttackRole, EEnemyAIAttackRole::CloseMelee);
	}
	const FGameplayTag RottenGuardLightAttackTag = FGameplayTag::RequestGameplayTag(TEXT("Enemy.Melee.LAtk1"), false);
	TestNotNull(TEXT("DA_RottenGuard has AbilityData"), RottenGuardData->AbilityData.Get());
	TestTrue(TEXT("Enemy.Melee.LAtk1 tag is registered"), RottenGuardLightAttackTag.IsValid());
	if (RottenGuardLightAttackTag.IsValid() && RottenGuardData->AbilityData)
	{
		TestTrue(TEXT("DA_RottenGuard AbilityData has valid LAtk1 montage or montage config"), RottenGuardData->AbilityData->HasAbility(RottenGuardLightAttackTag));
		const FEnemyAIAttackOption* LightAttack = FindAttackByTag(RottenGuardData->AttackProfile, RottenGuardLightAttackTag);
		TestNotNull(TEXT("DA_RottenGuard syncs valid LAtk1 montage tag into attack profile"), LightAttack);
		if (LightAttack)
		{
			TestEqual(TEXT("DA_RottenGuard LAtk1 is close melee role"), LightAttack->AttackRole, EEnemyAIAttackRole::CloseMelee);
		}
	}
	const FEnemyAIAttackOption* HeavyAttack = RottenGuardData->AttackProfile.Attacks.FindByPredicate(
		[](const FEnemyAIAttackOption& Attack)
		{
			return Attack.AttackName == TEXT("Heavy");
		});
	TestNotNull(TEXT("DA_RottenGuard has Heavy attack"), HeavyAttack);
	if (HeavyAttack)
	{
		TestEqual(TEXT("DA_RottenGuard Heavy is special movement role"), HeavyAttack->AttackRole, EEnemyAIAttackRole::SpecialMovement);
		TestEqual(TEXT("DA_RottenGuard Heavy uses radial lunge"), HeavyAttack->AttackMovementMode, EEnemyAIAttackMovementMode::RadialLunge);
		TestEqual(TEXT("DA_RottenGuard Heavy starts lunge from mid range"), HeavyAttack->LungeStartRange, 300.0f);
		TestEqual(TEXT("DA_RottenGuard Heavy lunge distance"), HeavyAttack->LungeDistance, 280.0f);
		TestEqual(TEXT("DA_RottenGuard Heavy max range"), HeavyAttack->MaxRange, 650.0f);
		TestEqual(TEXT("DA_RottenGuard Heavy movement attack range multiplier"), HeavyAttack->MovementAttackRangeMultiplier, 2.5f);
		TestEqual(TEXT("DA_RottenGuard Heavy movement attack cooldown"), HeavyAttack->MovementAttackCooldown, 10.0f);
	}

	return true;
}

#endif
