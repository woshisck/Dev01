// Fill out your copyright notice in the Description page of Project Settings.


#include "WeaponData.h"

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
