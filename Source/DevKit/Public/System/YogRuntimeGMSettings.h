#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "YogRuntimeGMSettings.generated.h"

class UEnemyData;
class UWeaponDefinition;
class UYogRuntimeGMWidget;

UCLASS(config = Game, defaultconfig, meta = (DisplayName = "Yog Runtime GM"))
class DEVKIT_API UYogRuntimeGMSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UYogRuntimeGMSettings();

	virtual FName GetCategoryName() const override;

	UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Runtime GM")
	bool bEnableRuntimeGM = true;

	UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Runtime GM")
	TSoftClassPtr<UYogRuntimeGMWidget> RuntimeGMWidgetClass;

	UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Runtime GM")
	TSoftObjectPtr<UWeaponDefinition> WeaponDefinition;

	UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Runtime GM")
	TSoftObjectPtr<UEnemyData> EnemyData;

	UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Runtime GM", meta = (ClampMin = "1", ClampMax = "50"))
	int32 SpawnCount = 1;

	UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Runtime GM", meta = (ClampMin = "100.0", ForceUnits = "cm"))
	float SpawnRadius = 1200.f;

	UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Runtime GM", meta = (ClampMin = "0.0", ForceUnits = "cm"))
	float SpawnMinDistance = 300.f;

	UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Runtime GM", meta = (ClampMin = "0.0", ForceUnits = "cm"))
	float SpawnZOffset = 96.f;

	UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Runtime GM")
	bool bSpawnedEnemiesCountForLevelClear = false;

	UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Runtime GM")
	bool bApplyRoomBuffsToSpawnedEnemies = false;
};
