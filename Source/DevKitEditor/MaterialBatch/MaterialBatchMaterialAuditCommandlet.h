#pragma once

#include "Commandlets/Commandlet.h"
#include "MaterialBatchMaterialAuditCommandlet.generated.h"

UCLASS()
class DEVKITEDITOR_API UMaterialBatchMaterialAuditCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UMaterialBatchMaterialAuditCommandlet();

	virtual int32 Main(const FString& Params) override;
};
