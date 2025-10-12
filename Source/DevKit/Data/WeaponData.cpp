// Fill out your copyright notice in the Description page of Project Settings.


#include "WeaponData.h"
#include "Character/YogCharacterBase.h"
#include "AbilitySystem/Abilities/YogGameplayAbility.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "Data/AbilityData.h"

const FWeaponAttributeData& UWeaponData::GetWeaponData() const
{
	if (!WeaponAttributeRow.IsNull())
	{
		FWeaponAttributeData* wpnData = WeaponAttributeRow.GetRow<FWeaponAttributeData>(__func__);
		if (wpnData)
		{
			return *wpnData;
		}
	}

	return DefaultWPNData;
	// TODO: insert return statement here
}


void UWeaponData::GrantAbilityToOwner(AYogCharacterBase* Owner)
{
	/*TArray<TObjectPtr<UAbilityData>> Action;*/
	UYogAbilitySystemComponent* asc = Owner->GetASC();
	if (Action.Num() > 0)
	{
		for (const UAbilityData* ability_action : Action)
		{
			//TSubclassOf<UYogGameplayAbility> ability;

			//ability_action->ability
			for (const TSubclassOf<UYogGameplayAbility> ability_class : ability_action->abilities)
			{
				FGameplayAbilitySpec abilitySpec(ability_class, 0);
				asc->GiveAbility(abilitySpec);
			}

		}
	}


}
