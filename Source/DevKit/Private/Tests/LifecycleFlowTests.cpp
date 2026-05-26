#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "BuffFlow/BuffFlowTypes.h"
#include "Data/AbilityData.h"
#include "Data/CharacterData.h"
#include "Data/EnemyData.h"

namespace LifecycleFlowTests
{
	static UClass* FindDevKitClass(const TCHAR* ClassName)
	{
		return FindObject<UClass>(nullptr, *FString::Printf(TEXT("/Script/DevKit.%s"), ClassName));
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLifecycleFlowContractsExposedTest,
	"DevKit.LifecycleFlow.ContractsExposed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLifecycleFlowContractsExposedTest::RunTest(const FString& Parameters)
{
	TestNotNull(TEXT("Spawn lifecycle flow asset type exists"),
		LifecycleFlowTests::FindDevKitClass(TEXT("SpawnLifecycleFlowAsset")));
	TestNotNull(TEXT("Spawn enemy from context node exists"),
		LifecycleFlowTests::FindDevKitClass(TEXT("BFNode_SpawnEnemyFromContext")));
	TestNotNull(TEXT("Finish lifecycle node exists"),
		LifecycleFlowTests::FindDevKitClass(TEXT("BFNode_FinishLifecycle")));

	const FObjectPropertyBase* SpawnFlowProperty = FindFProperty<FObjectPropertyBase>(
		UEnemyData::StaticClass(),
		TEXT("SpawnLifecycleFlow"));
	TestNotNull(TEXT("EnemyData exposes SpawnLifecycleFlow"), SpawnFlowProperty);

	const FObjectPropertyBase* DeathFlowProperty = FindFProperty<FObjectPropertyBase>(
		UAbilityData::StaticClass(),
		TEXT("DeathLifecycleFlow"));
	TestNull(TEXT("AbilityData no longer exposes DeathLifecycleFlow"), DeathFlowProperty);

	const FObjectPropertyBase* CharacterDeathFlowProperty = FindFProperty<FObjectPropertyBase>(
		UCharacterData::StaticClass(),
		TEXT("DeathLifecycleFlow"));
	TestNull(TEXT("CharacterData no longer exposes DeathLifecycleFlow"), CharacterDeathFlowProperty);

	const UEnum* TargetSelectorEnum = StaticEnum<EBFTargetSelector>();
	TestNotNull(TEXT("BuffFlow target selector enum exists"), TargetSelectorEnum);
	if (TargetSelectorEnum)
	{
		TestNotEqual(TEXT("BuffFlow target selector exposes LifecycleTarget"),
			TargetSelectorEnum->GetValueByNameString(TEXT("LifecycleTarget")),
			static_cast<int64>(INDEX_NONE));
	}

	return true;
}

#endif
