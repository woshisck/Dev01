#pragma once

#include "Commandlets/Commandlet.h"
#include "GraphicsSettingsWidgetSetupCommandlet.generated.h"

UCLASS()
class DEVKITEDITOR_API UGraphicsSettingsWidgetSetupCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UGraphicsSettingsWidgetSetupCommandlet();

	static FString GetTargetWidgetPackagePath();
	static FString GetReportFileName();

	virtual int32 Main(const FString& Params) override;
};
