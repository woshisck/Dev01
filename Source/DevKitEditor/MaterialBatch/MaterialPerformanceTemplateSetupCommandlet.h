#pragma once

#include "Commandlets/Commandlet.h"
#include "MaterialPerformanceTemplateSetupCommandlet.generated.h"

UCLASS()
class DEVKITEDITOR_API UMaterialPerformanceTemplateSetupCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UMaterialPerformanceTemplateSetupCommandlet();

	virtual int32 Main(const FString& Params) override;
};
