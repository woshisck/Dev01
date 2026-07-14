#pragma once

#include "Commandlets/Commandlet.h"
#include "StylizedCharacterMaterialSetupCommandlet.generated.h"

UCLASS()
class DEVKITEDITOR_API UStylizedCharacterMaterialSetupCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UStylizedCharacterMaterialSetupCommandlet();
	virtual int32 Main(const FString& Params) override;
};
