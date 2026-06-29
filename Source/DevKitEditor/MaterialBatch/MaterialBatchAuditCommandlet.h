#pragma once

#include "Commandlets/Commandlet.h"
#include "MaterialBatchAuditCommandlet.generated.h"

UCLASS()
class DEVKITEDITOR_API UMaterialBatchAuditCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UMaterialBatchAuditCommandlet();

	virtual int32 Main(const FString& Params) override;
};
