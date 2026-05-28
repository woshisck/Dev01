#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/Attribute/BaseAttributeSet.h"
#include "Character/EnemyCharacterBase.h"
#include "Character/TrainingDummyCharacter.h"
#include "Engine/GameInstance.h"
#include "EngineUtils.h"
#include "Map/RewardPickup.h"
#include "Map/TutorialMobSpawner.h"
#include "Mob/MobSpawner.h"
#include "Misc/ScopeExit.h"
#include "Story/Encounter/StoryEncounterPointDataAsset.h"

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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEnemyDeathDisappearDelayDefaultsToPostAnimationGraceTest,
	"DevKit.Enemy.DeathDisappearDelayDefaultsToPostAnimationGrace",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEnemyDeathDisappearDelayDefaultsToPostAnimationGraceTest::RunTest(const FString& Parameters)
{
	const AEnemyCharacterBase* DefaultEnemy = GetDefault<AEnemyCharacterBase>();
	TestNotNull(TEXT("Enemy default object exists"), DefaultEnemy);
	if (!DefaultEnemy)
	{
		return false;
	}

	const FFloatProperty* DelayProperty = FindFProperty<FFloatProperty>(
		AEnemyCharacterBase::StaticClass(),
		TEXT("DeathDisappearDelayAfterAnimation"));
	TestNotNull(TEXT("Enemy exposes death disappear delay after animation"), DelayProperty);
	if (!DelayProperty)
	{
		return false;
	}

	const float DefaultDelay = DelayProperty->GetPropertyValue_InContainer(DefaultEnemy);
	TestEqual(TEXT("Enemy default death disappear delay is 0.15s"), DefaultDelay, 0.15f);
	TestEqual(TEXT("Enemy death delay resolver ignores dissolve defaults"), DefaultEnemy->GetDeathDisappearDelayAfterAnimation(true), 0.15f);
	TestEqual(TEXT("Enemy death delay resolver handles non-dissolve deaths"), DefaultEnemy->GetDeathDisappearDelayAfterAnimation(false), 0.15f);
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

		if (UAbilitySystemComponent* ASC = SpawnedEnemy->GetAbilitySystemComponent())
		{
			if (ASC->HasAttributeSetForAttribute(UBaseAttributeSet::GetMaxHealthAttribute())
				&& ASC->HasAttributeSetForAttribute(UBaseAttributeSet::GetHealthAttribute()))
			{
				TestEqual(TEXT("Tutorial enemy max health override applied"),
					ASC->GetNumericAttribute(UBaseAttributeSet::GetMaxHealthAttribute()),
					Spawner->SpawnedMobMaxHealthOverride);
				TestEqual(TEXT("Tutorial enemy health override applied"),
					ASC->GetNumericAttribute(UBaseAttributeSet::GetHealthAttribute()),
					Spawner->SpawnedMobMaxHealthOverride);
			}
		}
		SpawnedEnemy->Destroy();
	}

	Spawner->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMobSpawnerStorySpawnUsesRegularSpawnerTest,
	"DevKit.MobSpawner.StorySpawnUsesRegularSpawner",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMobSpawnerStorySpawnUsesRegularSpawnerTest::RunTest(const FString& Parameters)
{
	UWorld* World = GWorld;
	TestNotNull(TEXT("Test world exists"), World);
	if (!World)
	{
		return false;
	}

	const FVector SpawnerLocation(321.f, 654.f, 87.f);
	AMobSpawner* Spawner = World->SpawnActor<AMobSpawner>(
		AMobSpawner::StaticClass(),
		SpawnerLocation,
		FRotator::ZeroRotator);
	TestNotNull(TEXT("Regular mob spawner spawned"), Spawner);
	if (!Spawner)
	{
		return false;
	}

	Spawner->SpawnZOffset = 96.f;
	FStoryMobSpawnOptions Options;
	Options.EnemyClassOverride = AEnemyCharacterBase::StaticClass();
	Options.bSpawnAtSpawnerLocation = true;
	Options.bCountsForLevelClear = false;
	Options.bRespawnOnDeath = false;

	AEnemyCharacterBase* SpawnedEnemy = Spawner->SpawnMobForStory(Options);
	TestNotNull(TEXT("Regular spawner can story-spawn an enemy"), SpawnedEnemy);
	if (SpawnedEnemy)
	{
		TestFalse(TEXT("Story-spawned enemy can opt out of level clear"), SpawnedEnemy->bCountsForLevelClear);
		TestEqual(TEXT("Story-spawned enemy uses spawner location plus Z offset"),
			SpawnedEnemy->GetActorLocation(),
			SpawnerLocation + FVector(0.f, 0.f, Spawner->SpawnZOffset));
	}

	Spawner->ClearStorySpawnedMob();
	Spawner->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMobSpawnerStoryKillEncounterTriggersOnDeathStartTest,
	"DevKit.MobSpawner.StoryKillEncounterTriggersOnDeathStart",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMobSpawnerStoryKillEncounterTriggersOnDeathStartTest::RunTest(const FString& Parameters)
{
	UGameInstance* GameInstance = NewObject<UGameInstance>(GEngine);
	GameInstance->InitializeStandalone(TEXT("MobSpawnerStoryKillEncounterTest"));
	UWorld* World = GameInstance->GetWorld();
	ON_SCOPE_EXIT
	{
		GameInstance->Shutdown();
		if (World)
		{
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(false);
		}
	};

	TestNotNull(TEXT("Standalone test world exists"), World);
	if (!World)
	{
		return false;
	}

	for (TActorIterator<ARewardPickup> It(World); It; ++It)
	{
		It->Destroy();
	}

	AMobSpawner* Spawner = World->SpawnActor<AMobSpawner>();
	TestNotNull(TEXT("Story mob spawner spawned"), Spawner);
	if (!Spawner)
	{
		return false;
	}

	UStoryEncounterPointDA* KillPoint = NewObject<UStoryEncounterPointDA>();
	KillPoint->EncounterId = TEXT("EM_Test");
	KillPoint->NodeId = TEXT("dummy_kill_immediate_drop");
	KillPoint->Kind = EStoryEncounterNodeKind::Death;
	KillPoint->FirePolicy = EStoryEncounterFirePolicy::Repeat;

	FStoryEncounterAction DropAction;
	DropAction.Kind = EStoryEncounterActionKind::SpawnRewardPickup;
	DropAction.RewardPickupClass = ARewardPickup::StaticClass();
	DropAction.RewardPickupCount = 1;
	DropAction.bRewardPickupAllowedOutsideArrangement = true;
	DropAction.bSpawnRewardOnTargetDeath = false;
	DropAction.bPlayRewardPickupFocusCue = false;
	FLootOption LootOption;
	LootOption.LootType = ELootType::Rune;
	LootOption.DisplayName = FText::FromString(TEXT("Immediate Heavy"));
	DropAction.RewardLootOptions.Add(LootOption);
	KillPoint->Actions.Add(DropAction);

	FStoryMobSpawnOptions Options;
	Options.EnemyClassOverride = ATrainingDummyCharacter::StaticClass();
	Options.bSpawnAtSpawnerLocation = true;
	Options.bCountsForLevelClear = false;
	Options.bRespawnOnDeath = false;
	Options.OnKillEncounterPoint = KillPoint;

	AEnemyCharacterBase* SpawnedEnemy = Spawner->SpawnMobForStory(Options);
	TestNotNull(TEXT("Story dummy spawned"), SpawnedEnemy);
	if (!SpawnedEnemy)
	{
		Spawner->Destroy();
		return false;
	}

	SpawnedEnemy->Die();

	ARewardPickup* ImmediatePickup = nullptr;
	for (TActorIterator<ARewardPickup> It(World); It; ++It)
	{
		ImmediatePickup = *It;
		break;
	}

	TestNotNull(TEXT("Kill encounter spawns reward immediately when Die starts"), ImmediatePickup);

	if (ImmediatePickup)
	{
		ImmediatePickup->Destroy();
	}
	if (SpawnedEnemy && !SpawnedEnemy->IsActorBeingDestroyed())
	{
		SpawnedEnemy->Destroy();
	}
	Spawner->Destroy();
	return true;
}

#endif
