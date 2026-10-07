#include "AbilitySystem/AbilityTask/MontagePhaseTagDriver.h"

#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"

namespace
{
	FGameplayTag PhaseTagForSection(const FName SectionName)
	{
		static const FGameplayTag TagPreAtk = FGameplayTag::RequestGameplayTag(TEXT("Character.State.Phase.PreAtk"), false);
		static const FGameplayTag TagAtk = FGameplayTag::RequestGameplayTag(TEXT("Character.State.Phase.Atk"), false);
		static const FGameplayTag TagPostAtk = FGameplayTag::RequestGameplayTag(TEXT("Character.State.Phase.PostAtk"), false);

		if (SectionName == FName(TEXT("PreAtk")))
		{
			return TagPreAtk;
		}
		if (SectionName == FName(TEXT("Atk")))
		{
			return TagAtk;
		}
		if (SectionName == FName(TEXT("PostAtk")))
		{
			return TagPostAtk;
		}

		return FGameplayTag();
	}
}

void FMontagePhaseTagDriver::Update(UAnimInstance& AnimInstance, UAnimMontage* Montage, UYogAbilitySystemComponent* ASC)
{
	if (!Montage)
	{
		Clear(ASC);
		return;
	}

	const FName SectionName = AnimInstance.Montage_GetCurrentSection(Montage);
	if (SectionName == CurrentSection)
	{
		return;
	}

	CurrentSection = SectionName;
	const FGameplayTag NewPhaseTag = PhaseTagForSection(SectionName);
	if (NewPhaseTag == CurrentPhaseTag)
	{
		return;
	}

	static const FName DefaultSection(TEXT("Default"));
	if (!NewPhaseTag.IsValid() && !SectionName.IsNone() && SectionName != DefaultSection)
	{
		UE_LOG(LogTemp, Warning, TEXT("[AttackPhase] Montage=%s section '%s' maps to no phase tag; expected PreAtk/Atk/PostAtk."),
			*GetNameSafe(Montage), *SectionName.ToString());
	}

	Clear(ASC);
	if (ASC && NewPhaseTag.IsValid())
	{
		ASC->AddLooseGameplayTag(NewPhaseTag);
		CurrentPhaseTag = NewPhaseTag;
	}
}

void FMontagePhaseTagDriver::Clear(UYogAbilitySystemComponent* ASC)
{
	if (CurrentPhaseTag.IsValid() && ASC)
	{
		ASC->RemoveLooseGameplayTag(CurrentPhaseTag);
	}

	CurrentPhaseTag = FGameplayTag();
	CurrentSection = NAME_None;
}
