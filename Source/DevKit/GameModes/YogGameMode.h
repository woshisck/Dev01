// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ModularGameMode.h"
#include "YogGameMode.generated.h"

class AYogPlayerControllerBase;
class UYogGameRule;
class UYogSaveGame;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnFinishLevel);

UCLASS()
class DEVKIT_API AYogGameMode : public AModularGameModeBase
{
	GENERATED_BODY()
	

public:
	AYogGameMode(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void RestartPlayer(AController* NewPlayer) override;
	virtual void StartPlay() override;
	virtual void PostLogin(APlayerController* NewPlayer) override;

	UFUNCTION(BlueprintCallable, Category = "Player")
	void SpawnPlayerAtPlayerStart(APlayerCharacterBase* player, const FString& IncomingName);


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player")
	bool bAutoSpawnPlayer = false;

	UPROPERTY(BlueprintAssignable)
	FOnFinishLevel OnFinishLevel;

	UFUNCTION(BlueprintCallable, Category = "KillCount")
	void UpdateMonsterKillCount(int count);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "KillCount")
	int MonsterKillCount;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "KillCount")
	int TargetMonsterKill;


protected:
	void OnGameRuleLoaded(const UYogGameRule* CurrentGameRule);
};
