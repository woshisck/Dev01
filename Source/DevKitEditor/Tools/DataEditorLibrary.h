#pragma once

#include "CoreMinimal.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Data/RuneDataAsset.h"
#include "Modules/ModuleManager.h"

class UDataEditorLibrary
{
public:
	static TArray<URuneDataAsset*> GetAllRuneDAs()
	{
		TArray<URuneDataAsset*> Runes;

		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
		IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

		FARFilter Filter;
		Filter.ClassPaths.Add(URuneDataAsset::StaticClass()->GetClassPathName());
		Filter.bRecursiveClasses = true;

		TArray<FAssetData> AssetDataList;
		AssetRegistry.GetAssets(Filter, AssetDataList);

		for (const FAssetData& AssetData : AssetDataList)
		{
			if (URuneDataAsset* Rune = Cast<URuneDataAsset>(AssetData.GetAsset()))
			{
				Runes.Add(Rune);
			}
		}

		return Runes;
	}
};
