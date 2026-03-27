// Fill out your copyright notice in the Description page of Project Settings.


#include "Data/CharacterData.h"

const FMovementData* UCharacterData::GetMovementData() const
{
	const FMovementData* pMovementCharacterData = nullptr;
	if (!MovementDataRow.IsNull())
	{
		pMovementCharacterData = MovementDataRow.GetRow<FMovementData>(TEXT("UCharacterData::GetMovementData"));
	}

	return pMovementCharacterData;

}

const FYogBaseAttributeData* UCharacterData::GetBaseAttributeData() const
{
	const FYogBaseAttributeData* pCharacterStatsData = nullptr;
	if (!YogBaseAttributeDataRow.IsNull())
	{
		pCharacterStatsData = YogBaseAttributeDataRow.GetRow<FYogBaseAttributeData>(TEXT("UCharacterData::GetBaseAttributeData"));
	}

	return pCharacterStatsData;

}


const UAbilityData* UCharacterData::GetAbilityData() const
{
	const UAbilityData* pAbilityData = nullptr;
	if (!AbilityData.IsNull())
	{
		pAbilityData = AbilityData;
	}

	return pAbilityData;
}

const UGASTemplate* UCharacterData::GetGASTemplate() const
{
	const UGASTemplate* pGASTemplate = nullptr;
	if (!GasTemplate.IsNull())
	{
		pGASTemplate = GasTemplate;
	}

	return pGASTemplate;
}

const TArray<TSubclassOf<UAnimInstance>> UCharacterData::GetDefaultAnimeLayers() const
{
	const TArray<TSubclassOf<UAnimInstance>> result_array;
	if (DefaultAnimeLayers.Num() > 0)
	{
		return DefaultAnimeLayers;
	}
	
	return result_array;
}