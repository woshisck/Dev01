#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "AbilitySystem/Attribute/BaseAttributeSet.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "Character/EnemyCharacterBase.h"
#include "Component/CharacterDataComponent.h"
#include "Data/EnemyData.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	/** Spawned actors + ASCs for one poise test; Destroy() tears the world state back down. */
	struct FPoiseTestRig
	{
		AEnemyCharacterBase* Enemy = nullptr;
		AActor* SourceActor = nullptr;
		UYogAbilitySystemComponent* EnemyASC = nullptr;
		UYogAbilitySystemComponent* SourceASC = nullptr;

		bool IsValid() const { return Enemy && SourceActor && EnemyASC && SourceASC; }

		void Destroy()
		{
			if (Enemy)
			{
				Enemy->Destroy();
			}
			if (SourceActor)
			{
				SourceActor->Destroy();
			}
		}
	};
}

/** Builds an enemy with a live poise pool of MaxPoise and an attacker dealing PoiseDamage per hit. */
static FPoiseTestRig EnemyPoiseTests_MakeRig(UWorld* World, float MaxPoise, float PoiseDamage)
{
	FPoiseTestRig Rig;
	if (!World)
	{
		return Rig;
	}

	Rig.Enemy = World->SpawnActor<AEnemyCharacterBase>();
	Rig.SourceActor = World->SpawnActor<AActor>();
	if (!Rig.Enemy || !Rig.SourceActor)
	{
		return Rig;
	}

	Rig.EnemyASC = Rig.Enemy->GetASC();
	Rig.SourceASC = NewObject<UYogAbilitySystemComponent>(Rig.SourceActor);
	Rig.SourceActor->AddInstanceComponent(Rig.SourceASC);
	Rig.SourceASC->RegisterComponent();
	Rig.SourceASC->InitAbilityActorInfo(Rig.SourceActor, Rig.SourceActor);
	if (!Rig.EnemyASC)
	{
		return Rig;
	}
	Rig.EnemyASC->InitAbilityActorInfo(Rig.Enemy, Rig.Enemy);

	UBaseAttributeSet* AttributeSet = NewObject<UBaseAttributeSet>(Rig.Enemy);
	Rig.EnemyASC->AddSpawnedAttribute(AttributeSet);

	// MaxPoise is the refill target ReceiveDamage snaps back to when the pool breaks.
	Rig.EnemyASC->MaxPoise = MaxPoise;
	Rig.EnemyASC->SetNumericAttributeBase(UBaseAttributeSet::GetResilienceAttribute(), MaxPoise);

	Rig.SourceASC->CurrentActionPoiseDamage = PoiseDamage;

	return Rig;
}

static float EnemyPoiseTests_GetPoise(const UYogAbilitySystemComponent* ASC)
{
	return ASC->GetNumericAttribute(UBaseAttributeSet::GetResilienceAttribute());
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEnemyDataRecentlyDamagedStateDurationDefaultTest,
	"DevKit.Enemy.Poise.RecentlyDamagedStateDurationDefaultsToThreeSeconds",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEnemyDataRecentlyDamagedStateDurationDefaultTest::RunTest(const FString& Parameters)
{
	const UEnemyData* EnemyData = GetDefault<UEnemyData>();
	TestNotNull(TEXT("EnemyData default object exists"), EnemyData);
	if (!EnemyData)
	{
		return false;
	}

	TestEqual(TEXT("Enemy recently damaged state defaults to 3 seconds"), EnemyData->RecentlyDamagedStateDuration, 3.0f);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEnemyRecentlyDamagedStateWindowTest,
	"DevKit.Enemy.Poise.RecentlyDamagedStateWindow",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEnemyRecentlyDamagedStateWindowTest::RunTest(const FString& Parameters)
{
	UWorld* World = GWorld;
	TestNotNull(TEXT("Automation world exists"), World);
	if (!World)
	{
		return false;
	}

	// Poise damage well under the pool, so no hit in this test staggers.
	FPoiseTestRig Rig = EnemyPoiseTests_MakeRig(World, 100.0f, 1.0f);
	TestTrue(TEXT("Poise test rig built"), Rig.IsValid());
	if (!Rig.IsValid())
	{
		Rig.Destroy();
		return false;
	}

	Rig.EnemyASC->RecentlyDamagedStateDuration = 3.0f;

	const FGameplayTag RecentlyDamagedTag =
		FGameplayTag::RequestGameplayTag(TEXT("Buff.RecentlyDamaged"));

	for (int32 HitIndex = 0; HitIndex < 4; ++HitIndex)
	{
		Rig.EnemyASC->ReceiveDamage(Rig.SourceASC, 1.0f);
	}

	TestTrue(TEXT("Enemy has recently damaged state while the window is active"),
		Rig.EnemyASC->HasMatchingGameplayTag(RecentlyDamagedTag));

	const float RemainingTime = Rig.EnemyASC->GetRecentlyDamagedStateRemainingTime();
	TestTrue(TEXT("Recently damaged timer uses configured 3 second duration"),
		RemainingTime > 2.9f && RemainingTime <= 3.0f);

	Rig.EnemyASC->ClearRecentlyDamagedState();
	TestFalse(TEXT("Recently damaged state is removed when the window is cleared"),
		Rig.EnemyASC->HasMatchingGameplayTag(RecentlyDamagedTag));

	Rig.Destroy();

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPoisePoolDrainsWithoutStaggerTest,
	"DevKit.Enemy.Poise.PoolDrainsWithoutStagger",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPoisePoolDrainsWithoutStaggerTest::RunTest(const FString& Parameters)
{
	UWorld* World = GWorld;
	TestNotNull(TEXT("Automation world exists"), World);
	if (!World)
	{
		return false;
	}

	FPoiseTestRig Rig = EnemyPoiseTests_MakeRig(World, 100.0f, 15.0f);
	TestTrue(TEXT("Poise test rig built"), Rig.IsValid());
	if (!Rig.IsValid())
	{
		Rig.Destroy();
		return false;
	}

	Rig.EnemyASC->ReceiveDamage(Rig.SourceASC, 1.0f);
	TestEqual(TEXT("One 15 point hit drains a 100 pool to 85"),
		EnemyPoiseTests_GetPoise(Rig.EnemyASC), 85.0f);

	Rig.EnemyASC->ReceiveDamage(Rig.SourceASC, 1.0f);
	TestEqual(TEXT("Poise damage accumulates across hits"),
		EnemyPoiseTests_GetPoise(Rig.EnemyASC), 70.0f);

	Rig.Destroy();

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPoisePoolBreaksAndRefillsTest,
	"DevKit.Enemy.Poise.PoolBreaksAndRefills",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPoisePoolBreaksAndRefillsTest::RunTest(const FString& Parameters)
{
	UWorld* World = GWorld;
	TestNotNull(TEXT("Automation world exists"), World);
	if (!World)
	{
		return false;
	}

	FPoiseTestRig Rig = EnemyPoiseTests_MakeRig(World, 100.0f, 15.0f);
	TestTrue(TEXT("Poise test rig built"), Rig.IsValid());
	if (!Rig.IsValid())
	{
		Rig.Destroy();
		return false;
	}

	// 100 / 15 -> the 7th hit is the one that empties the pool.
	for (int32 HitIndex = 0; HitIndex < 6; ++HitIndex)
	{
		Rig.EnemyASC->ReceiveDamage(Rig.SourceASC, 1.0f);
	}
	TestEqual(TEXT("Six hits leave the pool at 10"),
		EnemyPoiseTests_GetPoise(Rig.EnemyASC), 10.0f);

	Rig.EnemyASC->ReceiveDamage(Rig.SourceASC, 1.0f);
	TestEqual(TEXT("The hit that empties the pool refills it to max"),
		EnemyPoiseTests_GetPoise(Rig.EnemyASC), 100.0f);

	Rig.Destroy();

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPoiseOverwhelmingHitClampsTest,
	"DevKit.Enemy.Poise.OverwhelmingHitClampsToMax",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPoiseOverwhelmingHitClampsTest::RunTest(const FString& Parameters)
{
	UWorld* World = GWorld;
	TestNotNull(TEXT("Automation world exists"), World);
	if (!World)
	{
		return false;
	}

	FPoiseTestRig Rig = EnemyPoiseTests_MakeRig(World, 100.0f, 200.0f);
	TestTrue(TEXT("Poise test rig built"), Rig.IsValid());
	if (!Rig.IsValid())
	{
		Rig.Destroy();
		return false;
	}

	Rig.EnemyASC->ReceiveDamage(Rig.SourceASC, 1.0f);
	TestEqual(TEXT("A 200 point hit on a 100 pool refills to max rather than going negative"),
		EnemyPoiseTests_GetPoise(Rig.EnemyASC), 100.0f);

	Rig.Destroy();

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPoiseDotDoesNotDrainTest,
	"DevKit.Enemy.Poise.SuppressedHitReactDoesNotDrainPool",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPoiseDotDoesNotDrainTest::RunTest(const FString& Parameters)
{
	UWorld* World = GWorld;
	TestNotNull(TEXT("Automation world exists"), World);
	if (!World)
	{
		return false;
	}

	FPoiseTestRig Rig = EnemyPoiseTests_MakeRig(World, 100.0f, 15.0f);
	TestTrue(TEXT("Poise test rig built"), Rig.IsValid());
	if (!Rig.IsValid())
	{
		Rig.Destroy();
		return false;
	}

	Rig.EnemyASC->ReceiveDamage(Rig.SourceASC, 1.0f, /*bSuppressHitReact=*/true);
	TestEqual(TEXT("DoT damage leaves the poise pool untouched"),
		EnemyPoiseTests_GetPoise(Rig.EnemyASC), 100.0f);

	Rig.Destroy();

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPoiseSweepDrainsEveryTargetTest,
	"DevKit.Enemy.Poise.SweepDrainsEveryTarget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPoiseSweepDrainsEveryTargetTest::RunTest(const FString& Parameters)
{
	UWorld* World = GWorld;
	TestNotNull(TEXT("Automation world exists"), World);
	if (!World)
	{
		return false;
	}

	// One attacker, three defenders, a single CurrentActionPoiseDamage assignment:
	// every target must take the full poise damage, not just the first one hit.
	FPoiseTestRig Attacker = EnemyPoiseTests_MakeRig(World, 100.0f, 15.0f);
	FPoiseTestRig Second = EnemyPoiseTests_MakeRig(World, 100.0f, 0.0f);
	FPoiseTestRig Third = EnemyPoiseTests_MakeRig(World, 100.0f, 0.0f);
	TestTrue(TEXT("Poise test rigs built"), Attacker.IsValid() && Second.IsValid() && Third.IsValid());
	if (!Attacker.IsValid() || !Second.IsValid() || !Third.IsValid())
	{
		Attacker.Destroy();
		Second.Destroy();
		Third.Destroy();
		return false;
	}

	Attacker.EnemyASC->ReceiveDamage(Attacker.SourceASC, 1.0f);
	Second.EnemyASC->ReceiveDamage(Attacker.SourceASC, 1.0f);
	Third.EnemyASC->ReceiveDamage(Attacker.SourceASC, 1.0f);

	TestEqual(TEXT("First sweep target drained"), EnemyPoiseTests_GetPoise(Attacker.EnemyASC), 85.0f);
	TestEqual(TEXT("Second sweep target drained by the same amount"), EnemyPoiseTests_GetPoise(Second.EnemyASC), 85.0f);
	TestEqual(TEXT("Third sweep target drained by the same amount"), EnemyPoiseTests_GetPoise(Third.EnemyASC), 85.0f);

	Attacker.Destroy();
	Second.Destroy();
	Third.Destroy();

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEnemyDataPushesRecentlyDamagedDurationToASCTest,
	"DevKit.Enemy.Poise.EnemyDataPushesRecentlyDamagedDurationToASC",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEnemyDataPushesRecentlyDamagedDurationToASCTest::RunTest(const FString& Parameters)
{
	UWorld* World = GWorld;
	TestNotNull(TEXT("Automation world exists"), World);
	if (!World)
	{
		return false;
	}

	const FTransform SpawnTransform = FTransform::Identity;
	AEnemyCharacterBase* Enemy = World->SpawnActorDeferred<AEnemyCharacterBase>(
		AEnemyCharacterBase::StaticClass(),
		SpawnTransform);
	TestNotNull(TEXT("Deferred enemy spawned"), Enemy);
	if (!Enemy)
	{
		return false;
	}

	UEnemyData* EnemyData = NewObject<UEnemyData>(Enemy);
	EnemyData->RecentlyDamagedStateDuration = 4.25f;
	if (UCharacterDataComponent* CharacterData = Enemy->FindComponentByClass<UCharacterDataComponent>())
	{
		CharacterData->SetCharacterData(EnemyData);
	}

	UGameplayStatics::FinishSpawningActor(Enemy, SpawnTransform);
	if (!Enemy->HasActorBegunPlay())
	{
		Enemy->DispatchBeginPlay();
	}

	UYogAbilitySystemComponent* EnemyASC = Enemy->GetASC();
	TestNotNull(TEXT("Enemy ASC exists"), EnemyASC);
	if (!EnemyASC)
	{
		Enemy->Destroy();
		return false;
	}

	TestEqual(TEXT("EnemyData config pushes recently damaged duration to ASC"),
		EnemyASC->RecentlyDamagedStateDuration,
		4.25f);

	Enemy->Destroy();

	return true;
}

#endif
