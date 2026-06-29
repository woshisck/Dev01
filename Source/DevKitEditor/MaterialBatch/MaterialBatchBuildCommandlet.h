#pragma once

#include "Commandlets/Commandlet.h"
#include "MaterialBatchBuildCommandlet.generated.h"

UCLASS()
class DEVKITEDITOR_API UMaterialBatchBuildCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UMaterialBatchBuildCommandlet();

	virtual int32 Main(const FString& Params) override;
};
