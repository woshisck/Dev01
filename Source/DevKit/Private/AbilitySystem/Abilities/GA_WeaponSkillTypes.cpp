#include "AbilitySystem/Abilities/GA_WeaponSkillTypes.h"

#include "AbilitySystemComponent.h"
#include "Data/AbilityData.h"
#include "Data/WeaponSkillDataAsset.h"

UGA_WeaponSkill_THSwordCombo::UGA_WeaponSkill_THSwordCombo(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	WeaponSkillTag = FGameplayTag::RequestGameplayTag(TEXT("Weapon.Skill.THSwordCombo"));
	WeaponSkillDisplayName = NSLOCTEXT("WeaponSkill", "THSwordCombo", "双手剑连段");
	WeaponSkillDescription = NSLOCTEXT(
		"WeaponSkill",
		"THSwordComboDescription",
		"多段连续战技。连段窗口内按顺序推进第 1～4 段；每一段都从该战技专属动作数据中读取对应蒙太奇，未配置的下一段不会触发。");
	RequiredMontageSlots = {
		FGameplayTag::RequestGameplayTag(TEXT("Character.State.Skill.WeaponSkill.Combo1")),
		FGameplayTag::RequestGameplayTag(TEXT("Character.State.Skill.WeaponSkill.Combo2")),
		FGameplayTag::RequestGameplayTag(TEXT("Character.State.Skill.WeaponSkill.Combo3")),
		FGameplayTag::RequestGameplayTag(TEXT("Character.State.Skill.WeaponSkill.Combo4")),
	};
	WeaponSkillDataClass = UWeaponSkill_THSwordComboDataAsset::StaticClass();

	// Preserve the existing hold/release behavior while giving this skill a
	// dedicated native implementation point for future sword-combo effects.
	bHoldMontageUntilInputRelease = true;
}

bool UGA_WeaponSkill_THSwordCombo::PrepareForInputActivation()
{
	const UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	const FGameplayTag CanComboTag =
		FGameplayTag::RequestGameplayTag(TEXT("Character.State.Window.CanCombo"), false);
	const FGameplayTag JustComboTag =
		FGameplayTag::RequestGameplayTag(TEXT("Character.State.Window.JustCombo"), false);
	const bool bComboWindowIsOpen = ASC
		&& ((CanComboTag.IsValid() && ASC->HasMatchingGameplayTag(CanComboTag))
			|| (JustComboTag.IsValid() && ASC->HasMatchingGameplayTag(JustComboTag)));

	return PrepareComboSlot(IsActive(), bComboWindowIsOpen, true);
}

bool UGA_WeaponSkill_THSwordCombo::PrepareComboSlot(
	bool bAbilityIsActive,
	bool bComboWindowIsOpen,
	bool bRequireConfiguredMontage)
{
	const int32 AvailableSlotCount = FMath::Clamp(RequiredMontageSlots.Num(), 1, 4);
	const int32 CandidateSlot = bAbilityIsActive && bComboWindowIsOpen
		? CurrentComboSlot + 1
		: 1;
	if (CandidateSlot < 1 || CandidateSlot > AvailableSlotCount)
	{
		return false;
	}

	const UWeaponSkillDataAsset* SkillData = GetEquippedWeaponSkillData();
	if (bRequireConfiguredMontage
		&& SkillData
		&& (!SkillData->AbilityData
			|| !SkillData->AbilityData->GetMontage(RequiredMontageSlots[CandidateSlot - 1])))
	{
		// An empty montage slot makes that stage unavailable. Reject it before
		// retriggering the GA so no cost/cooldown is committed for missing data.
		return false;
	}

	PreparedComboSlot = CandidateSlot;
	return true;
}

void UGA_WeaponSkill_THSwordCombo::ApplyPreparedComboTag()
{
	static const TCHAR* ComboTagNames[] = {
		TEXT("Character.State.Skill.WeaponSkill.Combo1"),
		TEXT("Character.State.Skill.WeaponSkill.Combo2"),
		TEXT("Character.State.Skill.WeaponSkill.Combo3"),
		TEXT("Character.State.Skill.WeaponSkill.Combo4"),
	};
	CurrentComboSlot = FMath::Clamp(PreparedComboSlot, 1, UE_ARRAY_COUNT(ComboTagNames));
	ConfigureWeaponSkillComboTag(ComboTagNames[CurrentComboSlot - 1]);
}

void UGA_WeaponSkill_THSwordCombo::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	ApplyPreparedComboTag();
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

UGA_WeaponSkill_Block::UGA_WeaponSkill_Block(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	WeaponSkillTag = FGameplayTag::RequestGameplayTag(TEXT("Weapon.Skill.Block"));
	WeaponSkillDisplayName = NSLOCTEXT("WeaponSkill", "Block", "格挡");
	WeaponSkillDescription = NSLOCTEXT(
		"WeaponSkill",
		"BlockDescription",
		"按住进入格挡、松开结束。当前专属动作数据配置格挡蒙太奇；减伤、精力消耗和格挡效果等差异参数应继续放在该战技的 C++ GA/专属 DA 中扩展。");
	RequiredMontageSlots = {
		FGameplayTag::RequestGameplayTag(TEXT("Character.State.Skill.WeaponSkill.Combo1")),
	};
	WeaponSkillDataClass = UWeaponSkill_BlockDataAsset::StaticClass();
	bHoldMontageUntilInputRelease = true;
}

UGA_WeaponSkill_Thrust::UGA_WeaponSkill_Thrust(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	WeaponSkillTag = FGameplayTag::RequestGameplayTag(TEXT("Weapon.Skill.Thrust"));
	WeaponSkillDisplayName = NSLOCTEXT("WeaponSkill", "Thrust", "突刺");
	WeaponSkillDescription = NSLOCTEXT(
		"WeaponSkill",
		"ThrustDescription",
		"单段突进攻击战技。使用一个专属蒙太奇；位移、伤害、破韧等差异参数应配置在该战技的 C++ GA/专属 DA 中。");
	RequiredMontageSlots = {
		FGameplayTag::RequestGameplayTag(TEXT("Character.State.Skill.WeaponSkill.Combo1")),
	};
	WeaponSkillDataClass = UWeaponSkill_ThrustDataAsset::StaticClass();
	bHoldMontageUntilInputRelease = false;
}
