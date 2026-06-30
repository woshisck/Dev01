#pragma once

#include "Commandlets/Commandlet.h"
#include "EnvBatchAutoTagCommandlet.generated.h"

UCLASS()
class DEVKITEDITOR_API UEnvBatchAutoTagCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UEnvBatchAutoTagCommandlet();

	virtual int32 Main(const FString& Params) override;
};
