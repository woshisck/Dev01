// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ModularGameMode.h"
#include "YogGameMode.generated.h"

class AYogPlayerControllerBase;
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

	//UFUNCTION(BlueprintNativeEvent, Category = Game)
	//virtual void HandleStartingNewPlayer(APlayerController* NewPlayer) override;

	UFUNCTION(BlueprintCallable, Category = "Player")
	void SpawnPlayerAtPlayerStart(APlayerCharacterBase* player, const FString& IncomingName);


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player")
	bool bAutoSpawnPlayer = false;

	UPROPERTY(BlueprintAssignable)
	FOnFinishLevel OnFinishLevel;

	DECLARE_EVENT(AYogGameMode, FFinishLevelEvent)

	FFinishLevelEvent FinishLevelEvent;

	FFinishLevelEvent& OnFinishLevelEvent()
	{
		return FinishLevelEvent;
	}

	UFUNCTION(BlueprintCallable, Category = "KillCount")
	void UpdateFinishLevel(int count);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "KillCount")
	int MonsterKillCount;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "KillCount")
	int RemainKillCount;

protected:
	virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;

};
