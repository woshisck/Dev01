// Copyright Epic Games, Inc. All Rights Reserved.

#include "YogWorldSettings.h"
#include "GameFramework/PlayerStart.h"
#include "EngineUtils.h"
#include "Misc/UObjectToken.h"

#include "Engine/AssetManager.h"

AYogWorldSettings::AYogWorldSettings(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

FPrimaryAssetId AYogWorldSettings::GetDefaultGameRule() const
{
	FPrimaryAssetId Result;
	if (!DefaultGameRule.IsNull())
	{
		Result = UAssetManager::Get().GetPrimaryAssetIdForPath(DefaultGameRule.ToSoftObjectPath());

		if (!Result.IsValid())
		{
			UE_LOG(LogTemp, Error, TEXT("%s.DefaultGameplayExperience is %s but that failed to resolve into an asset ID (you might need to add a path to the Asset Rules in your game feature plugin or project settings"),
				*GetPathNameSafe(this), *DefaultGameRule.ToString());
		}
	}
	return Result;
}


