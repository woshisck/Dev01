#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "GameplayTagAssetMigrationCommandlet.generated.h"

UCLASS()
class DEVKITEDITOR_API UGameplayTagAssetMigrationCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UGameplayTagAssetMigrationCommandlet();

	virtual int32 Main(const FString& Params) override;
};
