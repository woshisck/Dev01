#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "AbilitySystem/Attribute/DamageAttributeSet.h"
#include "Component/CombatItemComponent.h"
#include "GameplayEffect.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatItemDefaultSlotViewTest,
	"DevKit.CombatItem.DefaultSlotViewsExposeCountsAndSelection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatItemDefaultSlotViewTest::RunTest(const FString& Parameters)
{
	UCombatItemComponent* Component = NewObject<UCombatItemComponent>();

	FCombatItemConfig Oil;
	Oil.ItemId = TEXT("OilBottle");
	Oil.DisplayName = FText::FromString(TEXT("Oil"));
	Oil.EffectType = ECombatItemEffectType::OilBottle;
	Oil.MaxCharges = 2;
	Oil.InitialCharges = 2;
	Oil.Cooldown = 12.0f;

	FCombatItemConfig Thunder = Oil;
	Thunder.ItemId = TEXT("ThunderStone");
	Thunder.DisplayName = FText::FromString(TEXT("Thunder"));
	Thunder.EffectType = ECombatItemEffectType::ThunderStone;
	Thunder.Cooldown = 16.0f;

	FCombatItemConfig Smoke = Oil;
	Smoke.ItemId = TEXT("SmokeBomb");
	Smoke.DisplayName = FText::FromString(TEXT("Smoke"));
	Smoke.EffectType = ECombatItemEffectType::SmokeBomb;
	Smoke.Cooldown = 18.0f;

	Component->SetSlotsForTest({Oil, Thunder, Smoke});

	TArray<FCombatItemSlotView> Views = Component->GetSlotViews();
	TestEqual(TEXT("Item bar has three slots"), Views.Num(), 3);
	TestTrue(TEXT("First slot is selected by default"), Views[0].bSelected);
	TestEqual(TEXT("Oil starts with two charges"), Views[0].Charges, 2);

	Component->SelectNextItem();
	Views = Component->GetSlotViews();
	TestFalse(TEXT("First slot is no longer selected"), Views[0].bSelected);
	TestTrue(TEXT("Second slot becomes selected"), Views[1].bSelected);
	TestEqual(TEXT("Thunder cooldown uses config"), Views[1].CooldownDuration, 16.0f);

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
