// Copyright 2021 Mickael Daniel. All Rights Reserved.

#include "GASCompanionDeveloper.h"
#include "Modules/ModuleManager.h"

DEFINE_LOG_CATEGORY(LogGASCompanionDeveloper);

class FGASCompanionDeveloperModule : public IGASCompanionDeveloperModule
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};

void FGASCompanionDeveloperModule::StartupModule()
{
	UE_LOG(LogGASCompanionDeveloper, Log, TEXT("Startup GASCompanionDeveloperModule Module"))

}

void FGASCompanionDeveloperModule::ShutdownModule()
{
	UE_LOG(LogGASCompanionDeveloper, Log, TEXT("Shutdown GASCompanionDeveloperModule Module"))
}

IMPLEMENT_MODULE(FGASCompanionDeveloperModule, GASCompanionDeveloper)

#undef LOCTEXT_NAMESPACE
