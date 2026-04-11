#pragma once

#include "DevKit.h"
#include "Engine/GameInstance.h"
#include "Component/BackpackGridComponent.h"
#include "Data/RoomDataAsset.h"
#include "Item/Weapon/WeaponDefinition.h"

#include "YogGameInstanceBase.generated.h"


class AYogCharacterBase;
class UYogSaveGame;

// =========================================================
// 局内跑局状态快照（切关时写入，新关卡加载后恢复）
// =========================================================
USTRUCT()
struct FRunState
{
	GENERATED_BODY()

	// true = 有有效存档；false = 新局开始，不恢复
	UPROPERTY()
	bool bIsValid = false;

	UPROPERTY()
	float CurrentHP = 0.f;

	UPROPERTY()
	int32 CurrentGold = 0;

	// BackpackGridComponent 的热度阶段（0-3）
	UPROPERTY()
	int32 CurrentPhase = 0;

	// 非永久符文（永久符文由 BGC::BeginPlay 重新放置，无需保存）
	UPROPERTY()
	TArray<FPlacedRune> PlacedRunes;

	// 切关前装备的武器 DA（新关卡恢复时重新装备）
	UPROPERTY()
	TObjectPtr<UWeaponDefinition> EquippedWeaponDef;

	// 整理阶段已选但尚未放入格子的符文（新关卡恢复后放回 PendingRunes）
	UPROPERTY()
	TArray<FRuneInstance> PendingRunes;
};
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



USTRUCT()
struct FSequenceStep
{
	GENERATED_BODY()

	/** Delegate to call for this step */
	FTimerDelegate Delegate;

	/** Delay before calling this step (seconds) */
	float Interval = 0.0f;
};


USTRUCT(BlueprintType)
struct FLevelStateCount
{
	GENERATED_BODY()

public:
	FLevelStateCount(){}

	UPROPERTY(BlueprintReadOnly)
	int MonsterSlayCount = 0;

	UPROPERTY(BlueprintReadOnly)
	int TotalMonsterSpawn = 0;

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

	virtual void Init() override;
	virtual void Shutdown() override;


	//UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Map state")
	//FLevelStateCount MapStateCount;

	UPROPERTY(BlueprintAssignable, Category = "File system")
	FStartNewGame OnStartNewGame;

	UPROPERTY(BlueprintAssignable, Category = "File system")
	FOpenSaveFile OnOpenSaveFile;

	UPROPERTY(BlueprintAssignable, Category = "Level system")
	FEnterLevel OnEnterLevel;

	// 关卡切换时存储下一关楼层编号（OpenLevel 后 GameMode 重建时读取）
	UPROPERTY(BlueprintReadWrite, Category = "Campaign")
	int32 PendingNextFloor = 1;

	// 局内跑局状态快照（切关前写入，新关卡 BeginPlay 后恢复）
	UPROPERTY()
	FRunState PendingRunState;

	// 下一关要使用的房间配置（ActivatePortals 由传送门写入，StartLevelSpawning 读取）
	UPROPERTY()
	TObjectPtr<URoomDataAsset> PendingRoomData;

	// 清空跑局状态（玩家死亡时调用，使下一局从默认值开始）
	void ClearRunState();

	UPROPERTY(BlueprintAssignable, Category = "Level system")
	FFinishLevel OnFinishLevel;

	UPROPERTY(BlueprintAssignable, Category = "File system")
	FStartSaveFile OnStartSaveFile;

	UFUNCTION(BlueprintCallable, Category = Inventory)
	APlayerCharacterBase* GetPlayerCharacter();

	/** The slot name used for saving */
	UPROPERTY(BlueprintReadWrite, Category = Save)
	FString SaveSlot;

	/////////////////////////////////// AI STUFF //////////////////////////////////////
	/** Add a step: bind a member function or lambda */
	void AddStep(const FTimerDelegate& InDelegate, float Interval);

	/** Start sequence from beginning */
	void StartSequence();

	/** Force move to next step immediately */
	void ForceNext();

	void PlayCurrentStep();
	void AdvanceToNextStep();

	TArray<FSequenceStep> Steps;
	int32 CurrentIndex = 0;
	FTimerHandle StepTimerHandle;


	///////////////////////////////////////////////////////////////////////////////////

	/** The platform-specific user index */
	UPROPERTY(BlueprintReadWrite, Category = Save)
	int32 SaveUserIndex;

	UPROPERTY(BlueprintReadWrite, Category = Save)
	TObjectPtr<UYogSaveGame> PersistentSaveData;

	UPROPERTY()
	int32 CurrentMapScore = 0;

	UPROPERTY()
	int32 CurrentMapKills = 0;

	UPROPERTY()
	int32 TargetMapKills = 0;

	UFUNCTION(BlueprintCallable)
	void AddCurrentMapKill(int32 adder);


	/** Delegate called when the save game has been loaded/reset */
	//UPROPERTY(BlueprintAssignable, Category = Inventory)
	FOnSaveGameLoaded OnSaveGameLoaded;

	/** Native delegate for save game load/reset */
	FOnSaveGameLoadedNative OnSaveGameLoadedNative;


	// Function to open a map and then load a save game from a slot
	UFUNCTION(BlueprintCallable, Category = "Save System")
	void OpenMapAndLoadSave(const TSoftObjectPtr<UWorld> Level);

	// Function to be called when the map has been loaded, to load the save game
	void OnPostLoadMap(UWorld* World);


	UFUNCTION(BlueprintCallable, Category = Save)
	void SaveGame();

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


	/** Writes the current save game object to disk. The save to disk happens in a background thread*/
	UFUNCTION(BlueprintCallable)
	void SpawnMobInMap();


private:
	// The save slot to load after the map is opened
	FString PendingSaveSlot;

	// Flag to indicate we want to load a save after the map is opened
	bool bShouldLoadSaveAfterMap;

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




	UPROPERTY()
	int WaveCount;
	
	UPROPERTY()
	int MaxWaveCount;


	UPROPERTY()
	int SpawnCount;
	
	UPROPERTY()
	float IntervalTime;

	UPROPERTY()
	float StageDelay;

};