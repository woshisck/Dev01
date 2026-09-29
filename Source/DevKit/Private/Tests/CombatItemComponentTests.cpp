#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "AbilitySystem/Attribute/DamageAttributeSet.h"
#include "Component/CombatItemComponent.h"
#include "GameplayEffect.h"

namespace
{
	FCombatItemConfig CombatItemTests_MakeItem(const FName ItemId, const FString& DisplayName)
	{
		FCombatItemConfig Config;
		Config.ItemId = ItemId;
		Config.DisplayName = FText::FromString(DisplayName);
		return Config;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatItemSlotViewTest,
	"DevKit.CombatItem.SlotViewsExposeIdentityAndSelection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatItemSlotViewTest::RunTest(const FString& Parameters)
{
	UCombatItemComponent* Component = NewObject<UCombatItemComponent>();

	Component->SetSlotsForTest({
		CombatItemTests_MakeItem(TEXT("ItemA"), TEXT("A")),
		CombatItemTests_MakeItem(TEXT("ItemB"), TEXT("B")),
		CombatItemTests_MakeItem(TEXT("ItemC"), TEXT("C")),
	});

	TArray<FCombatItemSlotView> Views = Component->GetSlotViews();
	TestEqual(TEXT("Item bar has three slots"), Views.Num(), 3);
	TestTrue(TEXT("First slot is selected by default"), Views[0].bSelected);
	TestEqual(TEXT("Slot keeps its authored id"), Views[0].ItemId, FName(TEXT("ItemA")));

	Component->SelectNextItem();
	Views = Component->GetSlotViews();
	TestFalse(TEXT("First slot is no longer selected"), Views[0].bSelected);
	TestTrue(TEXT("Second slot becomes selected"), Views[1].bSelected);

	Component->SelectPreviousItem();
	TestEqual(TEXT("Selection wraps back to the first slot"), Component->GetActiveSlotIndex(), 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatItemUseDoesNotConsumeOnFailureTest,
	"DevKit.CombatItem.FailedUseDoesNotConsumeTheItem",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatItemUseDoesNotConsumeOnFailureTest::RunTest(const FString& Parameters)
{
	UCombatItemComponent* Component = NewObject<UCombatItemComponent>();
	Component->SetSlotsForTest({CombatItemTests_MakeItem(TEXT("ItemA"), TEXT("A"))});

	// No ActivationTag authored, and no owning ASC to dispatch to: a single-use item must
	// survive both failures rather than being silently spent.
	TestFalse(TEXT("Use fails without an activation tag"), Component->UseActiveItem());
	TestEqual(TEXT("Failed use left the slot in place"), Component->GetSlotViews().Num(), 1);

	FCombatItemConfig Tagged = CombatItemTests_MakeItem(TEXT("ItemB"), TEXT("B"));
	Tagged.ActivationTag = FGameplayTag::RequestGameplayTag(TEXT("GameplayEvent.CombatItem.Throw"), false);
	if (!Tagged.ActivationTag.IsValid())
	{
		AddError(TEXT("GameplayEvent.CombatItem.Throw gameplay tag is missing."));
		return false;
	}

	Component->SetSlotsForTest({Tagged});
	TestFalse(TEXT("Use fails with no ability system to handle the event"), Component->UseActiveItem());
	TestEqual(TEXT("Failed dispatch left the slot in place"), Component->GetSlotViews().Num(), 1);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatItemNoHitReactTagTest,
	"DevKit.CombatItem.NoHitReactDamageTagIsRecognized",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatItemNoHitReactTagTest::RunTest(const FString& Parameters)
{
	UGameplayEffect* DamageGE = NewObject<UGameplayEffect>();
	DamageGE->DurationPolicy = EGameplayEffectDurationType::Instant;

	FGameplayEffectContextHandle Context;
	FGameplayEffectSpec Spec(DamageGE, Context, 1.0f);
	const FGameplayTag NoHitReactTag = FGameplayTag::RequestGameplayTag(TEXT("Item.Damage.NoHitReact"), false);
	if (!NoHitReactTag.IsValid())
	{
		AddError(TEXT("Item.Damage.NoHitReact gameplay tag is missing."));
		return false;
	}

	TestFalse(TEXT("Spec without item tag is not suppressed"), UCombatItemComponent::IsNoHitReactItemDamage(Spec));
	Spec.AddDynamicAssetTag(NoHitReactTag);
	TestTrue(TEXT("Spec with item tag suppresses hit react"), UCombatItemComponent::IsNoHitReactItemDamage(Spec));

	return true;
}

#endif
