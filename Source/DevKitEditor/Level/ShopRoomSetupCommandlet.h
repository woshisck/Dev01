#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "ShopRoomSetupCommandlet.generated.h"

UCLASS()
class DEVKITEDITOR_API UShopRoomSetupCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UShopRoomSetupCommandlet();

	virtual int32 Main(const FString& Params) override;
};
