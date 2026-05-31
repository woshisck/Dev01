#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "FirstRunTutorialRoomPoolSetupCommandlet.generated.h"

UCLASS()
class DEVKITEDITOR_API UFirstRunTutorialRoomPoolSetupCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UFirstRunTutorialRoomPoolSetupCommandlet();

	virtual int32 Main(const FString& Params) override;
};
