#pragma once

#include "Commandlets/Commandlet.h"
#include "BackpackDeckUIStyleSetupCommandlet.generated.h"

UCLASS()
class DEVKITEDITOR_API UBackpackDeckUIStyleSetupCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UBackpackDeckUIStyleSetupCommandlet();

	virtual int32 Main(const FString& Params) override;
};
