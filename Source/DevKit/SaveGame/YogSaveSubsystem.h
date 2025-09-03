// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "YogSaveSubsystem.generated.h"


class UYogSaveGame;


/// <summary>
/// TODO: ONLY ONE SLOT AVALIABLE FOR NOW, FOR GAME DESIGN MECHANICS ;)
/// </summary>
UCLASS()
class DEVKIT_API UYogSaveSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:
	void SerializeObject(UObject* Object, TArray<uint8>& OutResult);
	void DeserializeObject(const TArray<uint8>& Data, UObject* Object);

public:
	UFUNCTION(BlueprintCallable, Category = "My Game|Save Game")
	void LoadGame();
	
	UFUNCTION(BlueprintCallable, Category = "My Game|Save Game")
	void CreateNewSaveGame();

	UFUNCTION(BlueprintCallable, Category = "My Game|Save Game")
	void SaveCurrentSlot();



	UPROPERTY()
	uint8 DefaultUserIndex_SOLID = 0;

	UPROPERTY()
	FString SlotName = TEXT("Yogger");
private:
	UPROPERTY()
	TObjectPtr<UYogSaveGame> CurrentSaveGame;

	//save current state 
	void PopulateCurrentSlot();

	void PopulateFromCurrentSlot();
};
