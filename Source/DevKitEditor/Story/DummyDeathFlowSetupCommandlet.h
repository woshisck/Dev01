#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "DummyDeathFlowSetupCommandlet.generated.h"

UCLASS()
class DEVKITEDITOR_API UDummyDeathFlowSetupCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UDummyDeathFlowSetupCommandlet();

	virtual int32 Main(const FString& Params) override;
};
