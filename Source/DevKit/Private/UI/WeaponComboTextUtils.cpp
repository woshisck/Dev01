#include "UI/WeaponComboTextUtils.h"

#include "Data/AbilityData.h"
#include "Item/Weapon/WeaponDefinition.h"

namespace
{
	FString InputActionMarkup(const TCHAR* InputActionName)
	{
		return FString::Printf(TEXT("<input action=\"%s\"/>"), InputActionName);
	}

	bool HasAnyAbilityData(TConstArrayView<const UAbilityData*> AbilityDataSources)
	{
		for (const UAbilityData* AbilityData : AbilityDataSources)
		{
			if (AbilityData)
			{
				return true;
			}
		}
		return false;
	}

	bool HasAbilityInAnyData(TConstArrayView<const UAbilityData*> AbilityDataSources, const FGameplayTag& AbilityTag)
	{
		for (const UAbilityData* AbilityData : AbilityDataSources)
		{
			if (AbilityData && AbilityTag.IsValid() && AbilityData->HasAbility(AbilityTag))
			{
				return true;
			}
		}
		return false;
	}

	bool HasConfiguredActionForBroadTag(TConstArrayView<const UAbilityData*> AbilityDataSources, const TCHAR* BroadTagName)
	{
		const FGameplayTag BroadTag = FGameplayTag::RequestGameplayTag(FName(BroadTagName), false);
		if (HasAbilityInAnyData(AbilityDataSources, BroadTag))
		{
			return true;
		}

		for (int32 ComboIndex = 1; ComboIndex <= 4; ++ComboIndex)
		{
			const FString TagName = FString::Printf(TEXT("%s.Combo%d"), BroadTagName, ComboIndex);
			const FGameplayTag ComboTag = FGameplayTag::RequestGameplayTag(FName(*TagName), false);
			if (HasAbilityInAnyData(AbilityDataSources, ComboTag))
			{
				return true;
			}
		}

		return false;
	}

	bool HasConfiguredAction(
		TConstArrayView<const UAbilityData*> AbilityDataSources,
		const TCHAR* BroadTagName,
		const TCHAR* LegacyBroadTagName = nullptr)
	{
		return HasConfiguredActionForBroadTag(AbilityDataSources, BroadTagName)
			|| (LegacyBroadTagName && HasConfiguredActionForBroadTag(AbilityDataSources, LegacyBroadTagName));
	}

	void AddActionSlotLine(
		TArray<FString>& Lines,
		TConstArrayView<const UAbilityData*> AbilityDataSources,
		const TCHAR* BroadTagName,
		const TCHAR* LegacyBroadTagName,
		const TCHAR* InputActionName,
		const TCHAR* Label,
		const TCHAR* Description,
		int32 MaxLines)
	{
		if (!HasConfiguredAction(AbilityDataSources, BroadTagName, LegacyBroadTagName) || (MaxLines > 0 && Lines.Num() >= MaxLines))
		{
			return;
		}

		Lines.Add(FString::Printf(
			TEXT("%s   %s  %s"),
			Label,
			*InputActionMarkup(InputActionName),
			Description));
	}
}

FText WeaponComboTextUtils::BuildComboHintText(
	const UWeaponDefinition* WeaponDefinition,
	int32 MaxLines,
	bool bCompactSpacing)
{
	if (!WeaponDefinition)
	{
		return FText::FromString(TEXT("拾取武器后显示动作槽。"));
	}

	TArray<FString> ActionSlotLines;
	const bool bLimitLines = MaxLines > 0;
	const int32 EffectiveMaxLines = bLimitLines ? FMath::Max(1, MaxLines) : 0;

	const UAbilityData* AttackSources[] = {
		WeaponDefinition->AttackAbilityData.Get(),
	};
	const UAbilityData* WeaponSkillSources[] = {
		WeaponDefinition->WeaponSkillAbilityData.Get(),
	};

	AddActionSlotLine(ActionSlotLines, MakeArrayView(AttackSources), TEXT("Character.State.Skill.Attack"), TEXT("PlayerState.AbilityCast.Attack"), TEXT("Attack"), TEXT("攻击"), TEXT("顺序结算攻击卡组"), EffectiveMaxLines);
	AddActionSlotLine(ActionSlotLines, MakeArrayView(WeaponSkillSources), TEXT("Character.State.Skill.WeaponSkill"), TEXT("PlayerState.AbilityCast.WeaponSkill"), TEXT("WeaponSkill"), TEXT("战技"), TEXT("结算战技槽并引爆连携"), EffectiveMaxLines);
	AddActionSlotLine(ActionSlotLines, MakeArrayView(AttackSources), TEXT("Character.State.Movement.Dash"), TEXT("PlayerState.AbilityCast.Dash"), TEXT("Dash"), TEXT("冲刺"), TEXT("结算冲刺槽"), EffectiveMaxLines);

	if ((EffectiveMaxLines == 0 || ActionSlotLines.Num() < EffectiveMaxLines) && HasAnyAbilityData(MakeArrayView(AttackSources)))
	{
		ActionSlotLines.Add(FString::Printf(
			TEXT("%s   %s  %s"),
			TEXT("技能"),
			*InputActionMarkup(TEXT("Skill")),
			TEXT("由玩家选择的主动技能槽")));
	}

	if (ActionSlotLines.IsEmpty())
	{
		ActionSlotLines.Add(TEXT("未配置动作槽数据。"));
	}

	return FText::FromString(FString::Join(ActionSlotLines, bCompactSpacing ? TEXT("\n") : TEXT("\n\n")));
}
