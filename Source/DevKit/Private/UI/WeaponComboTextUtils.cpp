#include "UI/WeaponComboTextUtils.h"

#include "Data/AbilityData.h"
#include "Item/Weapon/WeaponDefinition.h"

namespace
{
	FString InputActionMarkup(const TCHAR* InputActionName)
	{
		return FString::Printf(TEXT("<input action=\"%s\"/>"), InputActionName);
	}

	bool HasAbilityInAnyData(TConstArrayView<const UAbilityData*> AbilityDataSources, const FGameplayTag& AbilityTag)
	{
		if (!AbilityTag.IsValid())
		{
			return false;
		}

		for (const UAbilityData* AbilityData : AbilityDataSources)
		{
			if (AbilityData && AbilityData->HasAbility(AbilityTag))
			{
				return true;
			}
		}
		return false;
	}

	int32 CountConfiguredComboSteps(TConstArrayView<const UAbilityData*> AbilityDataSources, const TCHAR* TagPrefix)
	{
		int32 Count = 0;
		for (int32 ComboIndex = 1; ComboIndex <= 4; ++ComboIndex)
		{
			const FString TagName = FString::Printf(TEXT("%s.Combo%d"), TagPrefix, ComboIndex);
			const FGameplayTag ComboTag = FGameplayTag::RequestGameplayTag(FName(*TagName), false);
			if (HasAbilityInAnyData(AbilityDataSources, ComboTag))
			{
				Count = ComboIndex;
			}
		}
		return Count;
	}

	FString BuildRepeatedInputSequence(const TCHAR* InputActionName, int32 Count)
	{
		TArray<FString> Tokens;
		Tokens.Reserve(Count);
		for (int32 Index = 0; Index < Count; ++Index)
		{
			Tokens.Add(InputActionMarkup(InputActionName));
		}
		return FString::Join(Tokens, TEXT(" -> "));
	}

	void AddComboLine(
		TArray<FString>& MoveListLines,
		TConstArrayView<const UAbilityData*> AbilityDataSources,
		const TCHAR* TagPrefix,
		const TCHAR* InputActionName,
		int32 MaxLines)
	{
		const int32 ComboStepCount = CountConfiguredComboSteps(AbilityDataSources, TagPrefix);
		if (ComboStepCount <= 0 || (MaxLines > 0 && MoveListLines.Num() >= MaxLines))
		{
			return;
		}

		MoveListLines.Add(FString::Printf(
			TEXT("\u8fde\u6bb5 %02d   %s"),
			MoveListLines.Num() + 1,
			*BuildRepeatedInputSequence(InputActionName, ComboStepCount)));
	}
}

FText WeaponComboTextUtils::BuildComboHintText(
	const UWeaponDefinition* WeaponDefinition,
	int32 MaxLines,
	bool bCompactSpacing)
{
	if (!WeaponDefinition)
	{
		return FText::FromString(TEXT("\u62fe\u53d6\u6b66\u5668\u540e\u663e\u793a\u51fa\u62db\u8868\u3002"));
	}

	TArray<FString> MoveListLines;
	const bool bLimitLines = MaxLines > 0;
	const int32 EffectiveMaxLines = bLimitLines ? FMath::Max(1, MaxLines) : 0;
	const UAbilityData* LegacyAbilityData = WeaponDefinition->AbilityData.Get();

	const UAbilityData* AttackSources[] = {
		WeaponDefinition->AttackAbilityData.Get(),
		LegacyAbilityData,
	};
	const UAbilityData* WeaponSkillSources[] = {
		WeaponDefinition->WeaponSkillAbilityData.Get(),
		LegacyAbilityData,
	};
	const UAbilityData* SpecialSources[] = {
		WeaponDefinition->SpecialAbilityData.Get(),
		LegacyAbilityData,
	};

	AddComboLine(MoveListLines, MakeArrayView(AttackSources), TEXT("PlayerState.AbilityCast.Attack"), TEXT("Attack"), EffectiveMaxLines);
	AddComboLine(MoveListLines, MakeArrayView(AttackSources), TEXT("PlayerState.AbilityCast.Dash"), TEXT("Dash"), EffectiveMaxLines);
	AddComboLine(MoveListLines, MakeArrayView(WeaponSkillSources), TEXT("PlayerState.AbilityCast.WeaponSkill"), TEXT("WeaponSkill"), EffectiveMaxLines);
	AddComboLine(MoveListLines, MakeArrayView(SpecialSources), TEXT("PlayerState.AbilityCast.Special"), TEXT("Special"), EffectiveMaxLines);

	if (MoveListLines.IsEmpty())
	{
		MoveListLines.Add(TEXT("\u672a\u914d\u7f6e\u62db\u5f0f\u6570\u636e\u3002"));
	}

	return FText::FromString(FString::Join(MoveListLines, bCompactSpacing ? TEXT("\n") : TEXT("\n\n")));
}
