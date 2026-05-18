#pragma once

#include "Commandlets/Commandlet.h"
#include "SlotSelectWidgetSetupCommandlet.generated.h"

UCLASS()
class DEVKITEDITOR_API USlotSelectWidgetSetupCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	USlotSelectWidgetSetupCommandlet();

	virtual int32 Main(const FString& Params) override;
};
