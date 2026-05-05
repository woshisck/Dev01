#pragma once

#include "Commandlets/Commandlet.h"
#include "MainUISetupCommandlet.generated.h"

UCLASS()
class DEVKITEDITOR_API UMainUISetupCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UMainUISetupCommandlet();

	virtual int32 Main(const FString& Params) override;
};
