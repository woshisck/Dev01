#pragma once

#include "Commandlets/Commandlet.h"
#include "MaterialBatchParentMaterialSetupCommandlet.generated.h"

UCLASS()
class DEVKITEDITOR_API UMaterialBatchParentMaterialSetupCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UMaterialBatchParentMaterialSetupCommandlet();

	virtual int32 Main(const FString& Params) override;
};
