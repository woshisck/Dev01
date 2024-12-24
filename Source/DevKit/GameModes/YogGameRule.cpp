// Fill out your copyright notice in the Description page of Project Settings.


#include "YogGameRule.h"
#include "GameFeatureAction.h"

UYogGameRule::UYogGameRule()
{
}

void UYogGameRule::UpdateAssetBundleData()
{
	Super::UpdateAssetBundleData();

	for (UGameFeatureAction* Action : Actions)
	{
		if (Action)
		{
			Action->AddAdditionalAssetBundleData(AssetBundleData);
		}
	}
}
