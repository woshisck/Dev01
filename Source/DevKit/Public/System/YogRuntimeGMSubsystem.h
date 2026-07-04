#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "YogRuntimeGMSubsystem.generated.h"

class AEnemyCharacterBase;
class APlayerController;
class APlayerCharacterBase;
class AYogCharacterBase;
class UEnemyData;
class UWeaponDefinition;
class UYogRuntimeGMWidget;

UCLASS()
class DEVKIT_API UYogRuntimeGMSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	UFUNCTION(BlueprintCallable, Category = "Runtime GM")
	bool ToggleGMPanel(APlayerController* PlayerController);

	UFUNCTION(BlueprintCallable, Category = "Runtime GM")
	void CloseGMPanel(APlayerController* PlayerController);

	UFUNCTION(BlueprintCallable, Category = "Runtime GM")
	bool GiveConfiguredWeapon(APlayerController* PlayerController);

	UFUNCTION(BlueprintCallable, Category = "Runtime GM")
	int32 SpawnConfiguredEnemies(APlayerController* PlayerController, int32 OverrideCount = -1);

	UFUNCTION(BlueprintCallable, Category = "Runtime GM")
	int32 ResetPlayerAndEnemies(APlayerController* PlayerController);

	UFUNCTION(BlueprintCallable, Category = "Runtime GM")
	void SetRuntimeSpawnCount(int32 InSpawnCount);

	UFUNCTION(BlueprintCallable, Category = "Runtime GM")
	void SetRuntimeSpawnRadius(float InSpawnRadius);

	UFUNCTION(BlueprintPure, Category = "Runtime GM")
	int32 GetRuntimeSpawnCount() const { return RuntimeSpawnCount; }

	UFUNCTION(BlueprintPure, Category = "Runtime GM")
	float GetRuntimeSpawnRadius() const { return RuntimeSpawnRadius; }

	UFUNCTION(BlueprintPure, Category = "Runtime GM")
	FText GetLastStatusText() const { return LastStatus; }

	UFUNCTION(BlueprintPure, Category = "Runtime GM")
	bool IsGMPanelOpen() const;

	UFUNCTION(BlueprintPure, Category = "Runtime GM")
	UWeaponDefinition* LoadConfiguredWeapon() const;

	UFUNCTION(BlueprintPure, Category = "Runtime GM")
	UEnemyData* LoadConfiguredEnemyData() const;

private:
	APlayerController* ResolvePlayerController(APlayerController* PlayerController) const;
	APlayerCharacterBase* ResolvePlayerCharacter(APlayerController* PlayerController) const;
	void SetStatus(const FText& Status);
	void ResetCharacterForGM(AYogCharacterBase* Character) const;

	UPROPERTY()
	TObjectPtr<UYogRuntimeGMWidget> ActiveWidget;

	bool bActiveWidgetManagedByUIManager = false;

	int32 RuntimeSpawnCount = 1;
	float RuntimeSpawnRadius = 1200.f;
	FText LastStatus;
};
