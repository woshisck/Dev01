#pragma once

#include "Commandlets/Commandlet.h"
#include "YogArtCodeMaterialSetupCommandlet.generated.h"

UCLASS()
class DEVKITEDITOR_API UYogArtCodeMaterialSetupCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UYogArtCodeMaterialSetupCommandlet();

	virtual int32 Main(const FString& Params) override;
};
