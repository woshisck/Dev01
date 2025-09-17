// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ModularGameMode.h"
#include "YogGameMode.generated.h"

class AYogPlayerControllerBase;
class UYogGameRule;
/**
 * 
 */
UCLASS()
class DEVKIT_API AYogGameMode : public AModularGameModeBase
{
	GENERATED_BODY()
	

public:
	AYogGameMode(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void RestartPlayer(AController* NewPlayer) override;
	virtual void StartPlay() override;

	UFUNCTION(BlueprintCallable, Category = "Player")
	void SpawnPlayerAtPlayerStart(APlayerCharacterBase* player, const FString& IncomingName);

	UFUNCTION(BlueprintCallable, Category = "Player")
	void SpawnPlayerFromSaveData(UYogSaveGame* savedata);

	UFUNCTION(BlueprintCallable, Category = "Player")
	void SpawnAndPossessDefaultCharacter(APlayerController* PlayerController, const FTransform& SpawnTransform);


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player")
	bool bAutoSpawnPlayer = false;

protected:
	void OnGameRuleLoaded(const UYogGameRule* CurrentGameRule);
};
