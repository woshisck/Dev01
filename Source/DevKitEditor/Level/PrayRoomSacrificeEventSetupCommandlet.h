#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "PrayRoomSacrificeEventSetupCommandlet.generated.h"

UCLASS()
class DEVKITEDITOR_API UPrayRoomSacrificeEventSetupCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UPrayRoomSacrificeEventSetupCommandlet();

	virtual int32 Main(const FString& Params) override;
};
