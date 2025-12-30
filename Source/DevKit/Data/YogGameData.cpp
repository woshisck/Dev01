#include "YogGameData.h"

#include "DevKit/DevAssetManager.h"
UYogGameData::UYogGameData()
{
}

const UYogGameData& UYogGameData::UYogGameData::Get()
{
	return  UDevAssetManager::GetDevAssetManager()->GetGameData();
}