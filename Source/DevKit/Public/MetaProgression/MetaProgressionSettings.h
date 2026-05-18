#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "MetaProgressionSettings.generated.h"

class UDataTable;

UCLASS(config = Game, defaultconfig, meta = (DisplayName = "Meta Progression"))
class DEVKIT_API UMetaProgressionSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	virtual FName GetCategoryName() const override;

	UPROPERTY(config, EditAnywhere, Category = "Data Tables")
	TSoftObjectPtr<UDataTable> MetaUpgradeNodeTable;

	UPROPERTY(config, EditAnywhere, Category = "Data Tables")
	TSoftObjectPtr<UDataTable> MetaCurrencyRuleTable;
};
