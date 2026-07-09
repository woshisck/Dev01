#include "Animation/AN_SwitchWeapon.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystem/Abilities/GA_SwitchWeapon.h"
#include "Character/PlayerCharacterBase.h"
#include "Components/SkeletalMeshComponent.h"

namespace
{
	UGA_SwitchWeapon* FindActiveSwitchWeaponAbility(UAbilitySystemComponent* ASC)
	{
		if (!ASC)
		{
			return nullptr;
		}

		if (UGA_SwitchWeapon* SwitchAbility = Cast<UGA_SwitchWeapon>(ASC->GetAnimatingAbility()))
		{
			return SwitchAbility;
		}

		for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
		{
			if (!Spec.IsActive())
			{
				continue;
			}

			for (UGameplayAbility* AbilityInstance : Spec.GetAbilityInstances())
			{
				if (UGA_SwitchWeapon* SwitchAbility = Cast<UGA_SwitchWeapon>(AbilityInstance))
				{
					return SwitchAbility;
				}
			}
		}

		return nullptr;
	}
}

void UAN_SwitchWeapon::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (!MeshComp)
	{
		return;
	}

	APlayerCharacterBase* Player = Cast<APlayerCharacterBase>(MeshComp->GetOwner());
	if (!Player)
	{
		return;
	}

	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Player);
	if (UGA_SwitchWeapon* SwitchAbility = FindActiveSwitchWeaponAbility(ASC))
	{
		SwitchAbility->TryCommitWeaponSwitch();
		return;
	}

	static const FGameplayTag SwitchWeaponTag =
		FGameplayTag::RequestGameplayTag(TEXT("Character.State.Equipment.SwitchWeapon"), false);
	static const FGameplayTag LegacySwitchWeaponTag =
		FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.SwitchWeapon"), false);
	if (ASC
		&& ((SwitchWeaponTag.IsValid() && ASC->HasMatchingGameplayTag(SwitchWeaponTag))
			|| (LegacySwitchWeaponTag.IsValid() && ASC->HasMatchingGameplayTag(LegacySwitchWeaponTag))))
	{
		return;
	}

	Player->SwitchWeapon();
}

FString UAN_SwitchWeapon::GetNotifyName_Implementation() const
{
	return TEXT("Switch Weapon");
}
