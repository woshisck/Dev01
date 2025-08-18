// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilityData.h"

const FActionData& FAbilityType::GetAction() const
{
	if (!ActionRow.IsNull())
	{
		FActionData* actionData = ActionRow.GetRow<FActionData>(__func__);
		if (actionData)
		{
			return *actionData;
		}
	}
	return DefaultActionData;
}
