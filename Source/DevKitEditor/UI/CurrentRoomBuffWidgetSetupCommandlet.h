#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "CurrentRoomBuffWidgetSetupCommandlet.generated.h"

UCLASS()
class DEVKITEDITOR_API UCurrentRoomBuffWidgetSetupCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UCurrentRoomBuffWidgetSetupCommandlet();

	virtual int32 Main(const FString& Params) override;
};
