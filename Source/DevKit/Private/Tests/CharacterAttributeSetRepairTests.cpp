#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/Attribute/BaseAttributeSet.h"
#include "AbilitySystem/Attribute/DamageAttributeSet.h"
#include "AbilitySystem/Attribute/RuneAttributeSet.h"
#include "Character/YogCharacterBase.h"
#include "Engine/World.h"

namespace
{
	template <typename TAttributeSet>
	TAttributeSet* GetMutableRegisteredAttributeSet(UAbilitySystemComponent* ASC)
	{
		return ASC
			? const_cast<TAttributeSet*>(Cast<const TAttributeSet>(ASC->GetAttributeSet(TAttributeSet::StaticClass())))
			: nullptr;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCharacterAttributeSetRepairRelinksRegisteredSetsTest,
	"DevKit.Character.AttributeSetRepairRelinksRegisteredSets",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCharacterAttributeSetRepairRelinksRegisteredSetsTest::RunTest(const FString& Parameters)
{
	UWorld* World = GWorld;
	TestNotNull(TEXT("Test world exists"), World);
	if (!World)
	{
		return false;
	}

	AYogCharacterBase* Character = World->SpawnActor<AYogCharacterBase>();
	TestNotNull(TEXT("Character spawned"), Character);
	if (!Character)
	{
		return false;
	}

	UAbilitySystemComponent* ASC = Character->GetAbilitySystemComponent();
	TestNotNull(TEXT("Character has ASC"), ASC);
	if (!ASC)
	{
		Character->Destroy();
		return false;
	}

	Character->EnsureCoreAttributeSetsRegistered();
	UBaseAttributeSet* RegisteredBaseSet = GetMutableRegisteredAttributeSet<UBaseAttributeSet>(ASC);
	UDamageAttributeSet* RegisteredDamageSet = GetMutableRegisteredAttributeSet<UDamageAttributeSet>(ASC);
	URuneAttributeSet* RegisteredRuneSet = GetMutableRegisteredAttributeSet<URuneAttributeSet>(ASC);

	TestNotNull(TEXT("ASC has registered BaseAttributeSet"), RegisteredBaseSet);
	TestNotNull(TEXT("ASC has registered DamageAttributeSet"), RegisteredDamageSet);
	TestNotNull(TEXT("ASC has registered RuneAttributeSet"), RegisteredRuneSet);
	if (!RegisteredBaseSet || !RegisteredDamageSet || !RegisteredRuneSet)
	{
		Character->Destroy();
		return false;
	}

	Character->BaseAttributeSet = nullptr;
	Character->DamageAttributeSet = nullptr;
	Character->RuneAttributeSet = nullptr;

	Character->EnsureCoreAttributeSetsRegistered();

	TestTrue(TEXT("BaseAttributeSet relinks to the ASC registered set"),
		Character->BaseAttributeSet.Get() == RegisteredBaseSet);
	TestTrue(TEXT("DamageAttributeSet relinks to the ASC registered set"),
		Character->DamageAttributeSet.Get() == RegisteredDamageSet);
	TestTrue(TEXT("RuneAttributeSet relinks to the ASC registered set"),
		Character->RuneAttributeSet.Get() == RegisteredRuneSet);

	Character->Destroy();
	return true;
}

#endif
