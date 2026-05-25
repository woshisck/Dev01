#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Character/EnemyCharacterBase.h"
#include "Character/TrainingDummyCharacter.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTutorialMobSpawnerTrainingDummyCanDestroyTest,
	"DevKit.TutorialMobSpawner.TrainingDummyCanDestroyInsteadOfReset",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTutorialMobSpawnerTrainingDummyCanDestroyTest::RunTest(const FString& Parameters)
{
	UWorld* World = GWorld;
	TestNotNull(TEXT("Test world exists"), World);
	if (!World)
	{
		return false;
	}

	ATrainingDummyCharacter* Dummy = World->SpawnActor<ATrainingDummyCharacter>();
	TestNotNull(TEXT("Training dummy spawned"), Dummy);
	if (!Dummy)
	{
		return false;
	}

	bool bDeathBroadcast = false;
	Dummy->OnCharacterDiedNative.AddLambda([&bDeathBroadcast](AYogCharacterBase*)
	{
		bDeathBroadcast = true;
	});

	Dummy->bResetOnDeath = false;
	Dummy->FinishDying();

	TestTrue(TEXT("Training dummy still broadcasts death before destroy"), bDeathBroadcast);
	TestTrue(TEXT("Training dummy can be destroyed instead of resetting"), Dummy->IsActorBeingDestroyed());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEnemyDeathCountsForLevelClearByDefaultTest,
	"DevKit.TutorialMobSpawner.EnemyDeathCountsForLevelClearByDefault",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEnemyDeathCountsForLevelClearByDefaultTest::RunTest(const FString& Parameters)
{
	const AEnemyCharacterBase* DefaultEnemy = GetDefault<AEnemyCharacterBase>();
	TestNotNull(TEXT("Enemy default object exists"), DefaultEnemy);
	if (!DefaultEnemy)
	{
		return false;
	}

	TestTrue(TEXT("Regular enemies count for level clear by default"), DefaultEnemy->bCountsForLevelClear);
	return true;
}

#endif
