// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerData.h"


const FCameraMovementData& UPlayerData::GetCameraMove() const
{
	if (!CameraDataRow.IsNull())
	{
		FCameraMovementData* cameraData = CameraDataRow.GetRow<FCameraMovementData>(__func__);
		if (cameraData)
		{
			return *cameraData;
		}
	}

	return DefaultMovement;
}
