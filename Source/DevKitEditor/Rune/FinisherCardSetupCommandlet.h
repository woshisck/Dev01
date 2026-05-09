#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "FinisherCardSetupCommandlet.generated.h"

UCLASS()
class DEVKITEDITOR_API UFinisherCardSetupCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UFinisherCardSetupCommandlet();

	virtual int32 Main(const FString& Params) override;
};
