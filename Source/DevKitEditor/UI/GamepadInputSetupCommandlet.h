#pragma once

#include "Commandlets/Commandlet.h"
#include "GamepadInputSetupCommandlet.generated.h"

UCLASS()
class DEVKITEDITOR_API UGamepadInputSetupCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UGamepadInputSetupCommandlet();

	virtual int32 Main(const FString& Params) override;
};
