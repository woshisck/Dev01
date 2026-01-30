// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Data/EnemyData.h"
#include "MobSpawner.generated.h"

class AEnemyCharacterBase;

UCLASS()
class DEVKIT_API AMobSpawner : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AMobSpawner();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable)
	AEnemyCharacterBase* SpawnMob();

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool SingleSpawn = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawning")
	float SpawnRadius = 1000.f;

	FVector GetRandomReachablePoint();

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<TSubclassOf<AEnemyCharacterBase>> EnemySpawnClassis;



};
