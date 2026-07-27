#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Character/EnemyCharacterBase.h"
#include "Component/CharacterDataComponent.h"
#include "Editor.h"
#include "Framework/Docking/TabManager.h"
#include "Tools/EnemyManager/EnemyDataActorFactory.h"
#include "Tools/EnemyManager/SEnemyManagerWidget.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEnemyManagerWidgetContractTest,
	"DevKit.Enemy.Authoring.EditorWidgetContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEnemyManagerWidgetContractTest::RunTest(const FString& Parameters)
{
	TestEqual(
		TEXT("The confirmed formal enemy catalogue contains 13 required entries"),
		SEnemyManagerWidget::GetRequiredFormalPlaceholderCountForTesting(),
		13);
	TestEqual(
		TEXT("The test library starts with two optional utility enemies"),
		SEnemyManagerWidget::GetOptionalTestPlaceholderCountForTesting(),
		2);

	const TArray<FString> ExpectedFormalIds = {
		TEXT("Shadow"),
		TEXT("CorruptedRat"),
		TEXT("RatNest"),
		TEXT("RottenGuard"),
		TEXT("AlarmBellJailer"),
		TEXT("RatPunishedPrisoner"),
		TEXT("SpineMusketeer"),
		TEXT("TyrantPrisoner"),
		TEXT("IronCagePrisoner"),
		TEXT("GuardCaptain"),
		TEXT("WitheredBloomAlchemist"),
		TEXT("CorruptedOccultist"),
		TEXT("InquisitorDalsoHermann"),
	};
	TestEqual(
		TEXT("Formal catalogue IDs match the confirmed enemy list"),
		SEnemyManagerWidget::GetRequiredFormalEnemyIdsForTesting(),
		ExpectedFormalIds);

	const TArray<FString> ExpectedTestIds = {
		TEXT("Dummy"),
		TEXT("TrainingDummy"),
	};
	TestEqual(
		TEXT("Test catalogue keeps utility enemies separate"),
		SEnemyManagerWidget::GetOptionalTestEnemyIdsForTesting(),
		ExpectedTestIds);

	TestTrue(
		TEXT("Formal enemy assets are isolated under /Game/Code/Enemy/Definitions"),
		SEnemyManagerWidget::IsOfficialEnemyPathForTesting(
			TEXT("/Game/Code/Enemy/Definitions/Normal/RottenGuard/DA_EN_RottenGuard")));
	TestFalse(
		TEXT("Legacy enemy data is not treated as a formal asset"),
		SEnemyManagerWidget::IsOfficialEnemyPathForTesting(
			TEXT("/Game/Docs/Data/Enemy/RottenGuard/DA_RottenGuard")));
	TestTrue(
		TEXT("Utility enemies are isolated under /Game/Test/Enemy"),
		SEnemyManagerWidget::IsTestEnemyPathForTesting(
			TEXT("/Game/Test/Enemy/Utility/Dummy/DA_TEST_EN_Dummy")));

	const TSharedRef<SEnemyManagerWidget> EnemyWidget =
		SNew(SEnemyManagerWidget)
		.SkipInitialAssetScan(true);
	TestTrue(TEXT("Enemy editor constructs its library and inspector"), EnemyWidget->HasCorePanelsForTesting());

	TestTrue(
		TEXT("Enemy editor has its own standalone tab spawner"),
		FGlobalTabmanager::Get()->HasTabSpawner(FName(TEXT("DevKitEnemyManager"))));
	TestTrue(
		TEXT("Weapon editor remains a separate standalone tab spawner"),
		FGlobalTabmanager::Get()->HasTabSpawner(FName(TEXT("DevKitWeaponManager"))));

	UEnemyData* PlacementEnemy = NewObject<UEnemyData>(
		GetTransientPackage(),
		TEXT("EnemyManagerPlacementContract"),
		RF_Public | RF_Standalone);
	PlacementEnemy->EnemyClass = AEnemyCharacterBase::StaticClass();
	const FAssetData PlacementAsset(PlacementEnemy);

	UActorFactory* RegisteredFactory = GEditor
		? GEditor->FindActorFactoryByClass(UEnemyDataActorFactory::StaticClass())
		: nullptr;
	TestNotNull(
		TEXT("EnemyData actor factory is registered for the level viewport preflight"),
		RegisteredFactory);
	if (RegisteredFactory)
	{
		FText PlacementError;
		TestTrue(
			TEXT("Registered enemy factory accepts a configured EnemyData asset"),
			RegisteredFactory->CanCreateActorFrom(PlacementAsset, PlacementError));
		TestEqual(
			TEXT("Registered enemy factory resolves the EnemyData Blueprint class"),
			RegisteredFactory->GetDefaultActorClass(PlacementAsset),
			AEnemyCharacterBase::StaticClass());
	}

	UWorld* EditorWorld = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	TestNotNull(TEXT("Editor world exists for enemy placement"), EditorWorld);
	if (EditorWorld)
	{
		UEnemyDataActorFactory* ConfiguredFactory =
			NewObject<UEnemyDataActorFactory>(GetTransientPackage());
		ConfiguredFactory->Configure(PlacementEnemy);

		FActorSpawnParameters SpawnParameters;
		SpawnParameters.ObjectFlags = RF_Transient;
		SpawnParameters.bTemporaryEditorActor = true;
		AActor* PlacedActor = ConfiguredFactory->CreateActor(
			PlacementEnemy,
			EditorWorld->GetCurrentLevel(),
			FTransform(FVector(0.f, 0.f, -100000.f)),
			SpawnParameters);
		AEnemyCharacterBase* PlacedEnemy = Cast<AEnemyCharacterBase>(PlacedActor);
		TestNotNull(
			TEXT("Enemy factory creates the configured enemy actor"),
			PlacedEnemy);
		if (PlacedEnemy)
		{
			const UCharacterDataComponent* CharacterData =
				PlacedEnemy->GetCharacterDataComponent();
			TestTrue(
				TEXT("Placed enemy receives the dragged EnemyData"),
				CharacterData && CharacterData->GetCharacterData() == PlacementEnemy);
			EditorWorld->EditorDestroyActor(PlacedEnemy, true);
		}
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEnemyKillRewardAuthoringContractTest,
	"DevKit.Enemy.Authoring.KillRewardContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEnemyKillRewardAuthoringContractTest::RunTest(const FString& Parameters)
{
	const UEnemyData* Defaults = GetDefault<UEnemyData>();
	TestFalse(TEXT("Enemy kill rewards are opt-in"), Defaults->bEnableKillRewards);
	TestTrue(TEXT("Enemy kill reward list starts empty"), Defaults->KillRewards.IsEmpty());

	const FEnemyKillRewardEntry Entry;
	TestEqual(TEXT("Enemy reward drop chance defaults to guaranteed"), Entry.DropChance, 1.0f);
	TestEqual(TEXT("Enemy reward minimum quantity multiplier defaults to one"), Entry.MinQuantityMultiplier, 1);
	TestEqual(TEXT("Enemy reward maximum quantity multiplier defaults to one"), Entry.MaxQuantityMultiplier, 1);

	return true;
}

#endif
