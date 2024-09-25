// Copyright 2021 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "GSCDeveloperSettings.generated.h"

/**
 * General Settings for GAS Companion Plugin.
 */
UCLASS(Config="Game", defaultconfig, meta=(DisplayName="GAS Companion"))
class GASCOMPANION_API UGSCDeveloperSettings : public UObject
{
	GENERATED_BODY()

public:

	UGSCDeveloperSettings(const FObjectInitializer& ObjectInitializer);

	/**
	 * Turn this on to prevent GAS Companion module to initialize UAbilitySystemGlobals (InitGlobalData) in the plugin StartupModule method.
	 *
	 * InitGlobalData() might be invoked a bit too early otherwise (with GAS Companion's StartupModule). It is expected that if you set this option to true to use
	 * an AssetManager subclass where `UAbilitySystemGlobals::Get().InitGlobalData()` is called in `StartInitialLoading``
	 *
	 * You'll need to update `Project Settings -> Engine > General Settings > Asset Manager Class` to use your AssetManager subclass.
	 *
	 * GAS Companion provides one `GSCAssetManager` and the editor should ask you if you want to update the `Asset Manager Class` to use it if the current Manager class
	 * is using engine's default one.
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Ability System", meta=(DisplayName = "Prevent Ability System Global Data Initialization in Startup Module (Recommended)"))
	bool bPreventGlobalDataInitialization = false;
};
