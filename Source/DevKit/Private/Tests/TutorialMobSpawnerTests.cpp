#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Character/EnemyCharacterBase.h"
#include "Character/TrainingDummyCharacter.h"
#include "EngineUtils.h"
#include "Map/TutorialMobSpawner.h"

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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTutorialMobSpawnerActivateSpawnsAtSpawnerLocationTest,
	"DevKit.TutorialMobSpawner.ActivateSpawnsAtSpawnerLocation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTutorialMobSpawnerActivateSpawnsAtSpawnerLocationTest::RunTest(const FString& Parameters)
{
	UWorld* World = GWorld;
	TestNotNull(TEXT("Test world exists"), World);
	if (!World)
	{
		return false;
	}

	TSet<AEnemyCharacterBase*> ExistingEnemies;
	for (TActorIterator<AEnemyCharacterBase> It(World); It; ++It)
	{
		ExistingEnemies.Add(*It);
	}

	const FVector SpawnerLocation(1234.f, 567.f, 89.f);
	ATutorialMobSpawner* Spawner = World->SpawnActor<ATutorialMobSpawner>(
		ATutorialMobSpawner::StaticClass(),
		SpawnerLocation,
		FRotator::ZeroRotator);
	TestNotNull(TEXT("Tutorial spawner spawned"), Spawner);
	if (!Spawner)
	{
		return false;
	}

	Spawner->EnemySpawnClassis.Add(AEnemyCharacterBase::StaticClass());
	Spawner->SpawnZOffset = 96.f;
	Spawner->Activate();

	AEnemyCharacterBase* SpawnedEnemy = nullptr;
	for (TActorIterator<AEnemyCharacterBase> It(World); It; ++It)
	{
		if (!ExistingEnemies.Contains(*It))
		{
			SpawnedEnemy = *It;
			break;
		}
	}

	TestNotNull(TEXT("Activate spawns a tutorial enemy without relying on NavMesh projection"), SpawnedEnemy);
	if (SpawnedEnemy)
	{
		TestEqual(TEXT("Tutorial enemy does not count for level clear"), SpawnedEnemy->bCountsForLevelClear, false);
		TestEqual(TEXT("Tutorial enemy spawns at spawner location plus Z offset"),
			SpawnedEnemy->GetActorLocation(),
			SpawnerLocation + FVector(0.f, 0.f, Spawner->SpawnZOffset));
		SpawnedEnemy->Destroy();
	}

	Spawner->Destroy();
	return true;
}

#endif
