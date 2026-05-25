#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "FirstRunTutorialSpawnerSetupCommandlet.generated.h"

UCLASS()
class DEVKITEDITOR_API UFirstRunTutorialSpawnerSetupCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UFirstRunTutorialSpawnerSetupCommandlet();

	virtual int32 Main(const FString& Params) override;
};
