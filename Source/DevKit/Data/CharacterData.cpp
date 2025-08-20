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

const FCharacterData& UCharacterData::GetCharacterData() const
{
	if (!CharacterDataRow.IsNull())
	{
		FCharacterData* character_data = CharacterDataRow.GetRow<FCharacterData>(__func__);
		if (character_data)
		{
			return *character_data;
		}
	}
	return DefaultCharacterData;
}
