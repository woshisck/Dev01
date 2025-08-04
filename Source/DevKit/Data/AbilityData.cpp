// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilityData.h"

const FYogAbilityData& UAbilityData::GetAbilityData() const
{
	if (!AbilityDataRow.IsNull())
	{
		FYogAbilityData* abilityData = AbilityDataRow.GetRow<FYogAbilityData>(__func__);
		if (abilityData)
		{
			return *abilityData;
		}
	}

	return DefaultAbilityData;
	// TODO: insert return statement here
}
