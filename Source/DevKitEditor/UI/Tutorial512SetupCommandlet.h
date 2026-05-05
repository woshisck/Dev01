#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "Tutorial512SetupCommandlet.generated.h"

UCLASS()
class DEVKITEDITOR_API UTutorial512SetupCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UTutorial512SetupCommandlet();

	virtual int32 Main(const FString& Params) override;
};
