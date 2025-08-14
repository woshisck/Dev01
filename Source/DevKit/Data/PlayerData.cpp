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

const FPlayerStatData& UPlayerData::GetPlayerState() const
{
	if (!PlayerDataRow.IsNull())
	{
		FPlayerStatData* playerStateData = PlayerDataRow.GetRow<FPlayerStatData>(__func__);
		if (playerStateData)
		{
			return *playerStateData;
		}
	}
	return DefaultPlayerState;
}


