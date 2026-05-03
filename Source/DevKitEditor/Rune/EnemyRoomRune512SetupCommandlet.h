#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "EnemyRoomRune512SetupCommandlet.generated.h"

UCLASS()
class DEVKITEDITOR_API UEnemyRoomRune512SetupCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UEnemyRoomRune512SetupCommandlet();

	virtual int32 Main(const FString& Params) override;
};
