#pragma once

#include "Commandlets/Commandlet.h"
#include "WaterStyleAssetSetupCommandlet.generated.h"

UCLASS()
class DEVKITEDITOR_API UWaterStyleAssetSetupCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UWaterStyleAssetSetupCommandlet();

	virtual int32 Main(const FString& Params) override;
};
