#pragma once

#include "Commandlets/Commandlet.h"
#include "DummyTrainingLifecycleSetupCommandlet.generated.h"

UCLASS()
class DEVKITEDITOR_API UDummyTrainingLifecycleSetupCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UDummyTrainingLifecycleSetupCommandlet();

	virtual int32 Main(const FString& Params) override;
};
