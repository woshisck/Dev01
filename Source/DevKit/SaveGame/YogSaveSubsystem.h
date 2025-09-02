// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "YogSaveSubsystem.generated.h"


class UYogSaveGame;
/**
 * 
 */
UCLASS()
class DEVKIT_API UYogSaveSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:
	void SerializeObject(UObject* Object, TArray<uint8>& OutResult);
	void DeserializeObject(const TArray<uint8>& Data, UObject* Object);

public:
	UFUNCTION(BlueprintCallable, Category = "My Game|Save Game")
	void CreateNewSaveGame(FString SlotName);

	UFUNCTION(BlueprintCallable, Category = "My Game|Save Game")
	void SaveCurrentSlot();

	UPROPERTY()
	uint8 DefaultUserIndex = 0;
private:
	UPROPERTY()
	TObjectPtr<UYogSaveGame> CurrentSaveGame;

	//save current state 
	void PopulateCurrentSlot();

	void PopulateFromCurrentSlot();
};
