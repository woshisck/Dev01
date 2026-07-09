#pragma once

#include "Commandlets/Commandlet.h"
#include "YogArtMaterialMasterSetupCommandlet.generated.h"

UCLASS()
class DEVKITEDITOR_API UYogArtMaterialMasterSetupCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UYogArtMaterialMasterSetupCommandlet();

	virtual int32 Main(const FString& Params) override;
};
