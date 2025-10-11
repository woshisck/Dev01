// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "YogSaveSubsystem.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSaveGameSignature, class UYogSaveGame*, SaveObject);



class UYogSaveGame;



UCLASS()
class DEVKIT_API UYogSaveSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()


public:
	UFUNCTION(BlueprintCallable, Category = "SaveGame")
	void SavePlayer(UYogSaveGame* SaveGame);

	UFUNCTION(BlueprintCallable, Category = "SaveGame")
	void LoadPlayer(UYogSaveGame* SaveGame);

	UFUNCTION(BlueprintCallable, Category = "SaveGame")
	void LoadMap(UYogSaveGame* SaveGame);

	UFUNCTION(BlueprintCallable, Category = "SaveGame")
	void SaveMap(UYogSaveGame* SaveGame);


	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "General")
	FString SaveSlotName = "SaveGame_01";

	UPROPERTY()
	TObjectPtr<UYogSaveGame> CurrentSaveGame;

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;


	UFUNCTION(BlueprintCallable, Category = "SaveGame")
	void WriteSaveGame();

	/* Load from disk, optional slot name */
	UFUNCTION(BlueprintCallable, Category = "SaveGame")
	void LoadSaveGame(FString InSlotName = "");

	UPROPERTY(BlueprintAssignable)
	FOnSaveGameSignature OnSaveGameLoaded;

	UPROPERTY(BlueprintAssignable)
	FOnSaveGameSignature OnSaveGameWritten;

	UFUNCTION()
	void OnLevelLoaded(UWorld* LoadedWorld);

	UFUNCTION(BlueprintCallable)
	UYogSaveGame* GetCurrentSave();

	UFUNCTION()
	UYogSaveGame* CreateSaveGameInst();


	UFUNCTION()
	void SaveData(UObject* Object, UPARAM(ref) TArray<uint8>& Data);

	UFUNCTION()
	void LoadData(UObject* Object, UPARAM(ref) TArray<uint8>& Data);


	void SaveTransformData(FVector& saveGame_loc, FVector& target_loc, FRotator& saveGame_rotate, FRotator& target_rot);


private:

	void LoadLevelData(UYogSaveGame* SaveGame);

};


//UObject* loadObj = StaticLoadObject(UBlueprint::StaticClass(), NULL, TEXT("Blueprint'/Game/Code/Characters/B_PlayerBase.B_PlayerBase_C'"));
//if (loadObj != nullptr)
//{
//	PlayerBlueprintClass = loadObj->GetClass();
//	//UBlueprint* ubp = Cast<UBlueprint>(loadObj);
//	//AActor* spawnActor = GetWorld()->SpawnActor<AActor>(ubp->GeneratedClass);
//	//UE_LOG(LogClass, Log, TEXT("Success"));
//}
