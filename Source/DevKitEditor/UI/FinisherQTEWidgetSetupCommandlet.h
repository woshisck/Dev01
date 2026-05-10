#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "FinisherQTEWidgetSetupCommandlet.generated.h"

UCLASS()
class DEVKITEDITOR_API UFinisherQTEWidgetSetupCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UFinisherQTEWidgetSetupCommandlet();

	virtual int32 Main(const FString& Params) override;
};
