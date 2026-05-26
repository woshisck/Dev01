#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "AbilitySystem/Attribute/BaseAttributeSet.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "Character/EnemyCharacterBase.h"
#include "Character/PlayerCharacterBase.h"
#include "Engine/World.h"
#include "Projectile/SlashWaveProjectile.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSlashWaveTargetedBounceDefaultsTest,
	"DevKit.Projectile.SlashWave.TargetedBounceDefaults",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSlashWaveTargetedBounceDefaultsTest::RunTest(const FString& Parameters)
{
	const FSlashWaveProjectileRuntimeConfig Config;
	TestFalse(TEXT("Targeted bounce is opt-in by default"), Config.bEnableTargetedBounce);
	TestEqual(TEXT("Targeted bounce defaults to five redirects"), Config.TargetedBounceMaxCount, 5);
	TestEqual(TEXT("Targeted bounce default search radius"), Config.TargetedBounceSearchRadius, 650.f);
	TestEqual(TEXT("Targeted bounce default travel range"), Config.TargetedBounceMaxTravelDistance, 650.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSlashWaveTargetedBounceReflectsIncomingDirectionTest,
	"DevKit.Projectile.SlashWave.TargetedBounceReflectsIncomingDirection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSlashWaveTargetedBounceReflectsIncomingDirectionTest::RunTest(const FString& Parameters)
{
	const FVector ReflectedDirection = ASlashWaveProjectile::ResolveTargetedBounceReflection(
		FVector::XAxisVector,
		-FVector::XAxisVector);

	TestTrue(TEXT("Reflection returns the incoming angle mirrored around the hit normal"),
		FVector::DotProduct(ReflectedDirection, -FVector::XAxisVector) > 0.99f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSlashWaveTargetedBounceChoosesNearestEnemyTest,
	"DevKit.Projectile.SlashWave.TargetedBounceChoosesNearestEnemy",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSlashWaveTargetedBounceChoosesNearestEnemyTest::RunTest(const FString& Parameters)
{
	UWorld* World = GWorld;
	TestNotNull(TEXT("Automation world exists for targeted bounce test"), World);
	if (!World)
	{
		return false;
	}

	APlayerCharacterBase* Source = World->SpawnActor<APlayerCharacterBase>();
	AEnemyCharacterBase* FirstEnemy = World->SpawnActor<AEnemyCharacterBase>();
	AEnemyCharacterBase* NearEnemy = World->SpawnActor<AEnemyCharacterBase>();
	AEnemyCharacterBase* FarEnemy = World->SpawnActor<AEnemyCharacterBase>();
	ASlashWaveProjectile* Projectile = World->SpawnActor<ASlashWaveProjectile>(
		ASlashWaveProjectile::StaticClass(),
		FVector::ZeroVector,
		FRotator::ZeroRotator);

	TestNotNull(TEXT("Source player spawned"), Source);
	TestNotNull(TEXT("First enemy spawned"), FirstEnemy);
	TestNotNull(TEXT("Near enemy spawned"), NearEnemy);
	TestNotNull(TEXT("Far enemy spawned"), FarEnemy);
	TestNotNull(TEXT("Slash wave projectile spawned"), Projectile);
	if (!Source || !FirstEnemy || !NearEnemy || !FarEnemy || !Projectile)
	{
		if (Source)
		{
			Source->Destroy();
		}
		if (FirstEnemy)
		{
			FirstEnemy->Destroy();
		}
		if (NearEnemy)
		{
			NearEnemy->Destroy();
		}
		if (FarEnemy)
		{
			FarEnemy->Destroy();
		}
		if (Projectile)
		{
			Projectile->Destroy();
		}
		return false;
	}

	Source->SetActorLocation(FVector(-200.f, 0.f, 0.f));
	FirstEnemy->SetActorLocation(FVector(100.f, 0.f, 0.f));
	NearEnemy->SetActorLocation(FVector(220.f, 90.f, 0.f));
	FarEnemy->SetActorLocation(FVector(420.f, 300.f, 0.f));

	if (UYogAbilitySystemComponent* SourceASC = Source->GetASC())
	{
		Source->EnsureCoreAttributeSetsRegistered();
		SourceASC->InitAbilityActorInfo(Source, Source);
	}
	auto MarkEnemyAlive = [](AEnemyCharacterBase* Enemy)
	{
		if (Enemy)
		{
			Enemy->EnsureCoreAttributeSetsRegistered();
			if (UYogAbilitySystemComponent* EnemyASC = Enemy->GetASC())
			{
				EnemyASC->InitAbilityActorInfo(Enemy, Enemy);
				EnemyASC->SetNumericAttributeBase(UBaseAttributeSet::GetHealthAttribute(), 100.f);
			}
		}
	};
	MarkEnemyAlive(FirstEnemy);
	MarkEnemyAlive(NearEnemy);
	MarkEnemyAlive(FarEnemy);

	FSlashWaveProjectileRuntimeConfig Config;
	Config.Damage = 0.f;
	Config.bForcePureDamage = true;
	Config.MaxHitCount = 6;
	Config.bEnableTargetedBounce = true;
	Config.TargetedBounceMaxCount = 5;
	Config.TargetedBounceSearchRadius = 650.f;
	Config.TargetedBounceMaxTravelDistance = 650.f;
	Projectile->InitProjectileWithConfig(Source, Config);

	Projectile->ApplyImmediateHit(FirstEnemy);

	FVector ExpectedDirection = NearEnemy->GetActorLocation() - Projectile->GetActorLocation();
	ExpectedDirection.Z = 0.f;
	ExpectedDirection = ExpectedDirection.GetSafeNormal();

	TestTrue(TEXT("Targeted bounce redirects toward the nearest unhit enemy"),
		FVector::DotProduct(Projectile->GetCurrentTravelDirection(), ExpectedDirection) > 0.98f);
	TestEqual(TEXT("Initial hit plus one redirect counts one targeted bounce"), Projectile->GetTargetedBounceCount(), 1);

	Source->Destroy();
	FirstEnemy->Destroy();
	NearEnemy->Destroy();
	FarEnemy->Destroy();
	Projectile->Destroy();
	return true;
}

#endif
