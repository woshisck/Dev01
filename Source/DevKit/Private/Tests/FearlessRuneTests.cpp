#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "AbilitySystem/Attribute/BaseAttributeSet.h"
#include "BuffFlow/Nodes/BFNode_AddTag.h"
#include "BuffFlow/Nodes/BFNode_ApplyAttributeModifier.h"
#include "BuffFlow/Nodes/BFNode_ApplyEffect.h"
#include "BuffFlow/Nodes/BFNode_CompareFloat.h"
#include "Data/RuneDataAsset.h"
#include "FlowAsset.h"
#include "GameplayEffect.h"

namespace
{
constexpr float FearlessHealthThreshold = 0.75f;
constexpr float FearlessDmgTakenMultiplier = 1.5f;
constexpr const TCHAR* FearlessRunePath =
	TEXT("/Game/Docs/BuffDocs/Playtest_GA/Fearless/DA_Rune_Fearless.DA_Rune_Fearless");

bool IsDmgTakenVulnerabilityModifier(const FGameplayAttribute& Attribute, EGameplayModOp::Type ModOp, const float Value)
{
	return Attribute == UBaseAttributeSet::GetDmgTakenAttribute()
		&& ModOp == EGameplayModOp::Multiplicitive
		&& FMath::IsNearlyEqual(Value, FearlessDmgTakenMultiplier);
}

bool GameplayEffectHasFearlessVulnerability(const UGameplayEffect* Effect)
{
	if (!Effect)
	{
		return false;
	}

	for (const FGameplayModifierInfo& Modifier : Effect->Modifiers)
	{
		float Magnitude = 0.f;
		if (Modifier.ModifierMagnitude.GetStaticMagnitudeIfPossible(1.f, Magnitude)
			&& IsDmgTakenVulnerabilityModifier(Modifier.Attribute, Modifier.ModifierOp, Magnitude))
		{
			return true;
		}
	}

	return false;
}

bool GameplayEffectGrantsTag(const UGameplayEffect* Effect, const FGameplayTag& Tag)
{
	return Effect && Tag.IsValid() && Effect->InheritableOwnedTagsContainer.CombinedTags.HasTagExact(Tag);
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFearlessRuneLowHealthTradeoffTest,
	"DevKit.Rune.Fearless.LowHealthSuperArmorDamageTakenTradeoff",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFearlessRuneLowHealthTradeoffTest::RunTest(const FString& Parameters)
{
	URuneDataAsset* Rune = Cast<URuneDataAsset>(StaticLoadObject(URuneDataAsset::StaticClass(), nullptr, FearlessRunePath));
	TestNotNull(TEXT("Fearless rune data asset exists"), Rune);
	if (!Rune)
	{
		return false;
	}

	const FString Description = Rune->RuneInfo.RuneConfig.RuneDescription.ToString();
	TestTrue(TEXT("Fearless description mentions the 75% health threshold"), Description.Contains(TEXT("75%")));
	TestTrue(TEXT("Fearless description mentions the 50% incoming damage penalty"), Description.Contains(TEXT("50%")));
	TestFalse(TEXT("Fearless description no longer advertises the old 20% value"), Description.Contains(TEXT("20%")));

	UFlowAsset* Flow = Rune->RuneInfo.Flow.FlowAsset;
	TestNotNull(TEXT("Fearless rune has a flow asset"), Flow);
	if (!Flow)
	{
		return false;
	}

	const FGameplayTag SuperArmorTag = FGameplayTag::RequestGameplayTag(TEXT("Buff.Status.SuperArmor"), false);
	bool bHasHealthThresholdGate = false;
	bool bGrantsSuperArmor = false;
	bool bAppliesDmgTakenVulnerability = false;

	for (const TPair<FGuid, UFlowNode*>& Pair : Flow->GetNodes())
	{
		const UFlowNode* Node = Pair.Value;
		if (const UBFNode_CompareFloat* CompareFloat = Cast<UBFNode_CompareFloat>(Node))
		{
			const bool bLowHealthOperator = CompareFloat->Operator == EBFCompareOp::LessThan
				|| CompareFloat->Operator == EBFCompareOp::LessOrEqual;
			if (bLowHealthOperator && FMath::IsNearlyEqual(CompareFloat->B.Value, FearlessHealthThreshold))
			{
				bHasHealthThresholdGate = true;
			}
		}

		if (const UBFNode_AddTag* AddTag = Cast<UBFNode_AddTag>(Node))
		{
			if (AddTag->Tag == SuperArmorTag)
			{
				bGrantsSuperArmor = true;
			}
		}

		if (const UBFNode_ApplyAttributeModifier* ApplyAttr = Cast<UBFNode_ApplyAttributeModifier>(Node))
		{
			if (IsDmgTakenVulnerabilityModifier(ApplyAttr->Attribute, ApplyAttr->ModOp.GetValue(), ApplyAttr->Value.Value))
			{
				bAppliesDmgTakenVulnerability = true;
			}
		}

		if (const UBFNode_ApplyEffect* ApplyEffect = Cast<UBFNode_ApplyEffect>(Node))
		{
			const UGameplayEffect* Effect = ApplyEffect->Effect
				? ApplyEffect->Effect->GetDefaultObject<UGameplayEffect>()
				: nullptr;
			bGrantsSuperArmor |= GameplayEffectGrantsTag(Effect, SuperArmorTag);
			bAppliesDmgTakenVulnerability |= GameplayEffectHasFearlessVulnerability(Effect);
		}
	}

	TestTrue(TEXT("Fearless triggers from a 75% low-health gate"), bHasHealthThresholdGate);
	TestTrue(TEXT("Fearless grants SuperArmor after the low-health gate"), bGrantsSuperArmor);
	TestTrue(TEXT("Fearless applies DmgTaken x1.5 after the low-health gate"), bAppliesDmgTakenVulnerability);

	return true;
}

#endif
