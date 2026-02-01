// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ModularGameMode.h"
#include "YogGameMode.generated.h"

class AYogPlayerControllerBase;
class UYogSaveGame;
class AEnemyCharacterBase;
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnFinishLevel);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnMapClean);


DECLARE_DELEGATE(FCleanAllMobInMap);
DECLARE_DELEGATE(FSpawnMobStart);
DECLARE_DELEGATE(FSpawnMobFinish);

USTRUCT(BlueprintType)
struct FSpawnConfig
{
	GENERATED_BODY()

public:
	// Number of mobs to spawn in this wave
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 MaxCall = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float FirstDelay = 2.0;

	// Interval between each spawn in this wave (seconds)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Interval = 1.0f;

	// Optional delay before starting this wave
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float StartDelay = 0.0f;

	// Mob class to spawn
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<AActor> MobClass = nullptr;
};



UCLASS()
class DEVKIT_API AYogGameMode : public AModularGameModeBase
{
	GENERATED_BODY()
	

public:
	AYogGameMode(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void BeginPlay()override;
	virtual void RestartPlayer(AController* NewPlayer) override;
	virtual void StartPlay() override;
	virtual void PostLogin(APlayerController* NewPlayer) override;



	//UFUNCTION(BlueprintNativeEvent, Category = Game)
	//virtual void HandleStartingNewPlayer(APlayerController* NewPlayer) override;

	UFUNCTION(BlueprintCallable, Category = "Player")
	void SpawnPlayerAtPlayerStart(APlayerCharacterBase* player, const FString& IncomingName);


	///////////////////////////////  AI  ////////////////////////////////
	// Timer handle for repeated calls
	FTimerHandle SpawnTimerHandle;

	UPROPERTY(BlueprintAssignable) 
	FOnMapClean OnMapClean;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawning")
	FSpawnConfig SpawnConfig;


	// Function to call repeatedly
	void SpawnMob();

	void OnMobDestroyed(AActor* DestroyedActor);
	
	void StartNextSpawnCycle();

	///////////////////////////////  AI  ////////////////////////////////
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
	
	UPROPERTY()
	int32 Current_CallCount;
};
