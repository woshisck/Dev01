#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "Character/EnemyCharacterBase.h"
#include "Component/CharacterDataComponent.h"
#include "Data/EnemyData.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"

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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEnemyPoiseCounterUsesRecentlyDamagedStateWindowTest,
	"DevKit.Enemy.Poise.CounterUsesRecentlyDamagedStateWindow",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEnemyPoiseCounterUsesRecentlyDamagedStateWindowTest::RunTest(const FString& Parameters)
{
	UWorld* World = GWorld;
	TestNotNull(TEXT("Automation world exists"), World);
	if (!World)
	{
		return false;
	}

	AEnemyCharacterBase* Enemy = World->SpawnActor<AEnemyCharacterBase>();
	AActor* SourceActor = World->SpawnActor<AActor>();
	TestNotNull(TEXT("Enemy spawned"), Enemy);
	TestNotNull(TEXT("Source actor spawned"), SourceActor);
	if (!Enemy || !SourceActor)
	{
		if (Enemy)
		{
			Enemy->Destroy();
		}
		if (SourceActor)
		{
			SourceActor->Destroy();
		}
		return false;
	}

	UYogAbilitySystemComponent* EnemyASC = Enemy->GetASC();
	UYogAbilitySystemComponent* SourceASC = NewObject<UYogAbilitySystemComponent>(SourceActor);
	SourceActor->AddInstanceComponent(SourceASC);
	SourceASC->RegisterComponent();
	SourceASC->InitAbilityActorInfo(SourceActor, SourceActor);
	EnemyASC->InitAbilityActorInfo(Enemy, Enemy);

	EnemyASC->SuperArmorThreshold = 8;
	EnemyASC->RecentlyDamagedStateDuration = 3.0f;

	const FGameplayTag RecentlyDamagedTag =
		FGameplayTag::RequestGameplayTag(TEXT("Buff.Status.RecentlyDamaged"));

	for (int32 HitIndex = 0; HitIndex < 4; ++HitIndex)
	{
		EnemyASC->ReceiveDamage(SourceASC, 1.0f);
	}

	TestEqual(TEXT("Poise hit count accumulates inside recently damaged state window"), EnemyASC->GetPoiseHitCount(), 4);
	TestTrue(TEXT("Enemy has recently damaged state while counter window is active"),
		EnemyASC->HasMatchingGameplayTag(RecentlyDamagedTag));

	const float RemainingTime = EnemyASC->GetRecentlyDamagedStateRemainingTime();
	TestTrue(TEXT("Recently damaged state reset timer uses configured 3 second duration"),
		RemainingTime > 2.9f && RemainingTime <= 3.0f);

	EnemyASC->ClearRecentlyDamagedState();
	TestEqual(TEXT("Poise hit count resets when recently damaged state is cleared"), EnemyASC->GetPoiseHitCount(), 0);
	TestFalse(TEXT("Recently damaged state is removed when the counter window is cleared"),
		EnemyASC->HasMatchingGameplayTag(RecentlyDamagedTag));

	Enemy->Destroy();
	SourceActor->Destroy();

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
