#include "AbilitySystem/AbilityTask/MontagePhaseTagDriver.h"

#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"

namespace
{
	// Attack montages are split into these sections; the section the playhead is in IS the attack
	// phase, so boundaries follow the animation instead of duplicating it as authored numbers that
	// go stale on retime.
	FGameplayTag MontagePhaseTagDriver_PhaseTagForSection(const FName SectionName)
	{
		static const FGameplayTag TAG_PreAtk  = FGameplayTag::RequestGameplayTag(TEXT("Character.State.Phase.PreAtk"), false);
		static const FGameplayTag TAG_Atk     = FGameplayTag::RequestGameplayTag(TEXT("Character.State.Phase.Atk"), false);
		static const FGameplayTag TAG_PostAtk = FGameplayTag::RequestGameplayTag(TEXT("Character.State.Phase.PostAtk"), false);

		static const FName NAME_PreAtk(TEXT("PreAtk"));
		static const FName NAME_Atk(TEXT("Atk"));
		static const FName NAME_PostAtk(TEXT("PostAtk"));

		if (SectionName == NAME_PreAtk)
		{
			return TAG_PreAtk;
		}
		if (SectionName == NAME_Atk)
		{
			return TAG_Atk;
		}
		if (SectionName == NAME_PostAtk)
		{
			return TAG_PostAtk;
		}

		return FGameplayTag();
	}
}

void FMontagePhaseTagDriver::Update(UAnimInstance& AnimInstance, UAnimMontage* Montage, UYogAbilitySystemComponent* ASC)
{
	const FName SectionName = AnimInstance.Montage_GetCurrentSection(Montage);
	if (SectionName == CurrentSection)
	{
		return;
	}

	CurrentSection = SectionName;

	const FGameplayTag NewPhaseTag = MontagePhaseTagDriver_PhaseTagForSection(SectionName);
	if (NewPhaseTag == CurrentPhaseTag)
	{
		return;
	}

	// A montage with no authored sections carries the single reserved section 'Default', which is
	// not a mistake. Any other unmapped name almost always is one, and would otherwise silently
	// produce no phase at all.
	static const FName NAME_DefaultSection(TEXT("Default"));
	if (!NewPhaseTag.IsValid() && !SectionName.IsNone() && SectionName != NAME_DefaultSection)
	{
		UE_LOG(LogTemp, Warning, TEXT("[AttackPhase] Montage=%s section '%s' maps to no phase tag; expected PreAtk/Atk/PostAtk."),
			*GetNameSafe(Montage), *SectionName.ToString());
	}

	Clear(ASC);

	if (NewPhaseTag.IsValid() && ASC)
	{
		ASC->AddLooseGameplayTag(NewPhaseTag);
		CurrentPhaseTag = NewPhaseTag;
	}
}

void FMontagePhaseTagDriver::Clear(UYogAbilitySystemComponent* ASC)
{
	if (!CurrentPhaseTag.IsValid())
	{
		return;
	}

	if (ASC)
	{
		ASC->RemoveLooseGameplayTag(CurrentPhaseTag);
	}

	CurrentPhaseTag = FGameplayTag();
}
