// Fill out your copyright notice in the Description page of Project Settings.


#include "CharacterData.h"

const FMovementData& UCharacterData::GetMovement() const
{
	if (!MoveDataRow.IsNull())
	{
		FMovementData* itemPrice = MoveDataRow.GetRow<FMovementData>(__func__);
		if (itemPrice)
		{
			return *itemPrice;
		}
	}

	return DefaultMovement;
	// TODO: insert return statement here
}
