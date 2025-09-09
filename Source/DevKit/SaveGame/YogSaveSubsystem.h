// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "YogSaveSubsystem.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSaveGameSignature, class UYogSaveGame*, SaveObject);


class UYogSaveGame;



/// <summary>
/// TODO: ONLY ONE SLOT 
/// </summary>


UCLASS()
class DEVKIT_API UYogSaveSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()


public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "General")
	FString SaveSlotName = "SaveGame_01";

	UPROPERTY(Transient)
	TObjectPtr<UYogSaveGame> CurrentSaveGame;

	UFUNCTION(BlueprintCallable, Category = "SaveGame")
	void WriteSaveGame();

	/* Load from disk, optional slot name */
	void LoadSaveGame(FString InSlotName = "");

	UPROPERTY(BlueprintAssignable)
	FOnSaveGameSignature OnSaveGameLoaded;

	UPROPERTY(BlueprintAssignable)
	FOnSaveGameSignature OnSaveGameWritten;

	UFUNCTION(BlueprintCallable)
	UYogSaveGame* GetCurrentSave();

	UFUNCTION()
	UYogSaveGame* CreateSaveGameInst();


	UFUNCTION()
	void SaveData(UObject* Object, TArray<uint8>& Data);

	UFUNCTION()
	void LoadData(UObject* Object, UPARAM(ref) TArray<uint8>& Data);

	/* Initialize Subsystem, good moment to load in SaveGameSettings variables */
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;


};
