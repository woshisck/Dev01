#pragma once

#include "../DevKit.h"
#include "Engine/GameInstance.h"



#include "YogGameInstanceBase.generated.h"


class AYogCharacterBase;
class UYogSaveGame;
/**
 * Base class for GameInstance, should be blueprinted
 * Most games will need to make a game-specific subclass of GameInstance
 * Once you make a blueprint subclass of your native subclass you will want to set it to be the default in project settings
 */

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FStartNewGame);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOpenSaveFile);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FStartSaveFile);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FEnterLevel);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FFinishLevel);



USTRUCT(BlueprintType)
struct FLevelStateCount
{
	GENERATED_BODY()

public:
	FLevelStateCount(){}

	UPROPERTY(BlueprintReadOnly)
	int MonsterSlayCount;

	UPROPERTY(BlueprintReadOnly)
	int TotalMonsterSpawn;

	void Reset()
	{
		MonsterSlayCount = 0;
	};
};

UCLASS()
class DEVKIT_API UYogGameInstanceBase : public UGameInstance
{
	GENERATED_BODY()

public:
	// Constructor
	UYogGameInstanceBase();


	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Map state")
	FLevelStateCount MapStateCount;


	UPROPERTY(BlueprintAssignable, Category = "File system")
	FStartNewGame OnStartNewGame;

	UPROPERTY(BlueprintAssignable, Category = "File system")
	FOpenSaveFile OnOpenSaveFile;

	UPROPERTY(BlueprintAssignable, Category = "Level system")
	FEnterLevel OnEnterLevel;

	UPROPERTY(BlueprintAssignable, Category = "Level system")
	FFinishLevel OnFinishLevel;

	UPROPERTY(BlueprintAssignable, Category = "File system")
	FStartSaveFile OnStartSaveFile;

	UFUNCTION(BlueprintCallable, Category = Inventory)
	APlayerCharacterBase* GetPlayerCharacter();

	/** The slot name used for saving */
	UPROPERTY(BlueprintReadWrite, Category = Save)
	FString SaveSlot;

	/** The platform-specific user index */
	UPROPERTY(BlueprintReadWrite, Category = Save)
	int32 SaveUserIndex;

	UPROPERTY(BlueprintReadWrite, Category = Save)
	TObjectPtr<UYogSaveGame> PersistentSaveData;


	/** Delegate called when the save game has been loaded/reset */
	//UPROPERTY(BlueprintAssignable, Category = Inventory)
	FOnSaveGameLoaded OnSaveGameLoaded;

	/** Native delegate for save game load/reset */
	FOnSaveGameLoadedNative OnSaveGameLoadedNative;

	/**
	 * Adds the default inventory to the inventory array
	 * @param InventoryArray Inventory to modify
	 * @param RemoveExtra If true, remove anything other than default inventory
	 */
	UFUNCTION(BlueprintCallable, Category = Inventory)
	void AddDefaultInventory(UYogSaveGame* SaveGame, bool bRemoveExtra = false);

	/** Returns true if this is a valid inventory slot */
	//UFUNCTION(BlueprintCallable, Category = Inventory)
	//bool IsValidItemSlot(FRPGItemSlot ItemSlot) const;


	virtual void Init() override;

	/** Returns the current save game, so it can be used to initialize state. Changes are not written until WriteSaveGame is called */
	UFUNCTION(BlueprintCallable, Category = Save)
	UYogSaveGame* GetCurrentSaveGame();

	/** Sets rather save/load is enabled. If disabled it will always count as a new character */
	UFUNCTION(BlueprintCallable, Category = Save)
	void SetSavingEnabled(bool bEnabled);

	/** Synchronously loads a save game. If it fails, it will create a new one for you. Returns true if it loaded, false if it created one */
	UFUNCTION(BlueprintCallable, Category = Save)
	bool LoadOrCreateSaveGame();

	/** Handle the final setup required after loading a USaveGame object using AsyncLoadGameFromSlot. Returns true if it loaded, false if it created one */
	UFUNCTION(BlueprintCallable, Category = Save)
	bool HandleSaveGameLoaded(USaveGame* SaveGameObject);

	/** Gets the save game slot and user index used for inventory saving, ready to pass to GameplayStatics save functions */
	UFUNCTION(BlueprintCallable, Category = Save)
	void GetSaveSlotInfo(FString& SlotName, int32& UserIndex) const;

	/** Writes the current save game object to disk. The save to disk happens in a background thread*/
	UFUNCTION(BlueprintCallable, Category = Save)
	bool WriteSaveGame();


protected:

	/** The current save game object */
	UPROPERTY()
	UYogSaveGame* CurrentSaveGame;

	/** Rather it will attempt to actually save to disk */
	UPROPERTY()
	bool bSavingEnabled;

	/** True if we are in the middle of doing a save */
	UPROPERTY()
	bool bCurrentlySaving;

	/** True if another save was requested during a save */
	UPROPERTY()
	bool bPendingSaveRequested;

	/** Called when the async save happens */
	virtual void HandleAsyncSave(const FString& SlotName, const int32 UserIndex, bool bSuccess);
};