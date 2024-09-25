// Copyright 2021 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class SNotificationItem;

class FGSCModule : public IModuleInterface
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	/** Returns the path to the fully qualified documentation URL for the provided path. Path must start with starting a leading "/" */
	GASCOMPANION_API static FString GetDocumentationURL(FString InPath = "");

protected:
	TSharedPtr<SNotificationItem> SettingsNotificationItem;

	bool HandleSettingsSaved();
	void UpdateAssetManagerClass();
	
	void OnPostEngineInit();
};
