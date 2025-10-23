// Fill out your copyright notice in the Description page of Project Settings.


#include "CharacterData.h"

const FMovementData& UCharacterData::GetMovementData() const
{
	if (!MovementDataRow.IsNull())
	{
		FMovementData* itemPrice = MovementDataRow.GetRow<FMovementData>(__func__);
		if (itemPrice)
		{
			return *itemPrice;
		}
	}

	return DefaultMovementData;
	// TODO: insert return statement here
}

const FYogBaseAttributeData& UCharacterData::GetBaseAttributeData() const
{
	if (!YogBaseAttributeDataRow.IsNull())
	{
		FYogBaseAttributeData* character_data = YogBaseAttributeDataRow.GetRow<FYogBaseAttributeData>(__func__);
		if (character_data)
		{
			return *character_data;
		}
	}
	return DefaultCharacterData;
}
