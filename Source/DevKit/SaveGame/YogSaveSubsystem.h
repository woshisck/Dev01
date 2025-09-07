// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "YogSaveSubsystem.generated.h"


class UYogSaveGame;



/// <summary>
/// TODO: ONLY ONE SLOT AVALIABLE FOR NOW
/// </summary>


UCLASS()
class DEVKIT_API UYogSaveSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:
	UYogSaveSubsystem();

public:


	UFUNCTION(BlueprintCallable)
	void UObjectArraySaver(UPARAM(ref) TArray<UObject*>& SaveObjects);


	UFUNCTION(BlueprintCallable)
	void AutoSave();

	UFUNCTION(BlueprintCallable)
	void AutoLoad();



	UFUNCTION(BlueprintCallable)
	void SaveData(UObject* Object, TArray<uint8>& Data);

	UFUNCTION(BlueprintCallable)
	void LoadData(UObject* Object, UPARAM(ref) TArray<uint8>& Data);

	UPROPERTY()
	uint8 DefaultUserIndex_SOLID = 0;

	UPROPERTY()
	FString SlotName = TEXT("YoggorSave");
private:
	UPROPERTY()
	TObjectPtr<UYogSaveGame> CurrentSaveGame;

};
