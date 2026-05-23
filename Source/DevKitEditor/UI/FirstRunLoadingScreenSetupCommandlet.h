#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "FirstRunLoadingScreenSetupCommandlet.generated.h"

UCLASS()
class DEVKITEDITOR_API UFirstRunLoadingScreenSetupCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UFirstRunLoadingScreenSetupCommandlet();
	virtual int32 Main(const FString& Params) override;
};
