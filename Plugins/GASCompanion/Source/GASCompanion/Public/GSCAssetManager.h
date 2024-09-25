// Copyright 2021 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/AssetManager.h"
#include "GSCAssetManager.generated.h"

/**
 * Child of UAssetManager with the primary purpose of initializing UAbilitySystemGlobals GlobalData
 */
UCLASS()
class GASCOMPANION_API UGSCAssetManager : public UAssetManager
{
	GENERATED_BODY()

public:
	//~ UAssetManager interface
	/**
	 * Starts initial load, gets called from InitializeObjectReferences.
	 *
	 * Here, we ensure UAbilitySystemGlobals' InitGlobalData is called.
	 */
	virtual void StartInitialLoading() override;
	//~ End UAssetManager
};
