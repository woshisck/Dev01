#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "EnemyHealthMaterialSetupCommandlet.generated.h"

UCLASS()
class DEVKITEDITOR_API UEnemyHealthMaterialSetupCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UEnemyHealthMaterialSetupCommandlet();
	virtual int32 Main(const FString& Params) override;
};
