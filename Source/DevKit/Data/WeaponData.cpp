// Fill out your copyright notice in the Description page of Project Settings.


#include "WeaponData.h"
#include "Character/YogCharacterBase.h"
#include "AbilitySystem/Abilities/YogGameplayAbility.h"
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

void UWeaponData::SetupWeaponAttributeToOwner(AYogCharacterBase* Owner)
{
	const FWeaponAttributeData& wpnData = this->GetWeaponData();
	Owner->SetWeaponAttribute(wpnData);
}

void UWeaponData::GrantAbilityToOwner(AYogCharacterBase* Owner)
{
	//	TArray<TObjectPtr<UAbilityData>> Action;
	UYogAbilitySystemComponent* asc = Owner->GetASC();

	for (const UAbilityData* ability_data : Action)
	{
		//	TArray<FYogAbilityData> Abilities;
		for (const FYogAbilityData& yog_ability_data : ability_data->Abilities)
		{
			UYogGameplayAbility* ability = NewObject<UYogGameplayAbility>(this, yog_ability_data.Ability);
			if (ability)
			{
				//yog_ability_data.ActDamage
				//	yog_ability_data.ActRange
				//	yog_ability_data.ActResilience
				//	yog_ability_data.ActDmgReduce
				//	yog_ability_data.ActRotateSpeed
				//	yog_ability_data.JumpFrameTime
				//	yog_ability_data.FreezeFrameTime

				ability->SetupAbilityStat(yog_ability_data);
				FGameplayAbilitySpec abilitySpec(ability, 0);


			}

			//TSubclassOf<UYogGameplayAbility> AbilityToGrant, int32 AbilityLevel)
			//FGameplayAbilitySpec AbilitySpec(AbilityToGrant, AbilityLevel);


			//FGameplayAbilitySpec AbilitySpec(AbilityToGrant, AbilityLevel);
			//AbilitySystemComponent->GiveAbility(AbilitySpec);

			//asc->GiveAbility()
			//Owner->GrantGameplayAbility(ability_data.Ability, 0);


			//for (const UYogAbilitySet* YogAbilitiesSet : AbilitySetsToGrant)
			//{
			//	for (FYogAbilitySet_GameplayAbility GameAbilitySet : YogAbilitiesSet->GrantedGameplayAbilities)
			//	{
			//		ReceivingChar->GrantGameplayAbility(GameAbilitySet.Ability, GameAbilitySet.AbilityLevel);
			//	}
			//}


		}
	}

}
