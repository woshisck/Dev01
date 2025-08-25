// Fill out your copyright notice in the Description page of Project Settings.


#include "CharacterData.h"

const FMovementData& UCharacterData::GetMovementData() const
{
	if (!MoveDataRow.IsNull())
	{
		FMovementData* itemPrice = MoveDataRow.GetRow<FMovementData>(__func__);
		if (itemPrice)
		{
			return *itemPrice;
		}
	}

	return DefaultMovementData;
	// TODO: insert return statement here
}

const FYogBaseData& UCharacterData::GetCharacterData() const
{
	if (!CharacterDataRow.IsNull())
	{
		FYogBaseData* character_data = CharacterDataRow.GetRow<FYogBaseData>(__func__);
		if (character_data)
		{
			return *character_data;
		}
	}
	return DefaultCharacterData;
}
