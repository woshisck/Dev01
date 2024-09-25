// Copyright 2021 Mickael Daniel. All Rights Reserved.


#include "GSCAssetManager.h"

#include "AbilitySystemGlobals.h"
#include "GSCLog.h"

void UGSCAssetManager::StartInitialLoading()
{
	Super::StartInitialLoading();

	GSC_LOG(Display, TEXT("GAS Companion AssetManager (StartInitialLoading) - Initialize Ability System Component (InitGlobalData)"));
	UAbilitySystemGlobals::Get().InitGlobalData();
}
