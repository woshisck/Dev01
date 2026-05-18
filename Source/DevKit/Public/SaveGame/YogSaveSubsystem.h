// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "SaveGame/YogSaveGame.h"
#include "SaveGame/YogSettingsSave.h"
#include "YogSaveSubsystem.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSaveGameSignature, class UYogSaveGame*, SaveObject);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnSlotPreviewReady, const FSlotPreviewData&, Preview);


class UYogSaveGame;
class UYogSettingsSave;


UCLASS()
class DEVKIT_API UYogSaveSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	// =========================================================
	// 多槽位管理（3 个独立槽位，索引 0-2）
	// =========================================================

	// 激活并加载指定槽位（选档后调用）
	UFUNCTION(BlueprintCallable, Category = "SaveGame|Slot")
	void SelectSlot(int32 SlotIndex);

	// 删除指定槽位的存档文件
	UFUNCTION(BlueprintCallable, Category = "SaveGame|Slot")
	void DeleteSlot(int32 SlotIndex);

	// 新游戏重置：清空 MetaProgression + RunCheckpoint，保留 Statistics
	UFUNCTION(BlueprintCallable, Category = "SaveGame|Slot")
	void ResetSlotForNewGame(int32 SlotIndex);

	// 异步读取槽位预览（不加载全部数据，选档 UI 用）
	UFUNCTION(BlueprintCallable, Category = "SaveGame|Slot")
	void RequestSlotPreview(int32 SlotIndex, FOnSlotPreviewReady Callback);

	// 获取槽位存档文件名（"SaveSlot_0/1/2"）
	UFUNCTION(BlueprintPure, Category = "SaveGame|Slot")
	FString GetSlotName(int32 SlotIndex) const;

	// =========================================================
	// 存档点（Checkpoint）— 三个触发点
	// ①进入关卡前  ②清关后  触发此函数
	// =========================================================

	// 将当前 PendingRunState 序列化为 RunCheckpoint 并异步写盘
	UFUNCTION(BlueprintCallable, Category = "SaveGame|Checkpoint")
	void TriggerCheckpoint(int32 CurrentFloor);

	// 玩家死亡 / 结局正常结束时调用：清除存档点，异步写盘
	UFUNCTION(BlueprintCallable, Category = "SaveGame|Checkpoint")
	void ClearRunCheckpoint();

	// =========================================================
	// 快速存档 — 背包 UI 关闭时调用（异步，主线程无感知）
	// =========================================================

	UFUNCTION(BlueprintCallable, Category = "SaveGame|Checkpoint")
	void QuickSave();

	// =========================================================
	// 全局设置（独立存档，不绑定槽位）
	// =========================================================

	UFUNCTION(BlueprintCallable, Category = "SaveGame|Settings")
	void SaveSettings();

	UFUNCTION(BlueprintCallable, Category = "SaveGame|Settings")
	void LoadSettings();

	UFUNCTION(BlueprintPure, Category = "SaveGame|Settings")
	UYogSettingsSave* GetSettings() const { return CurrentSettings; }

	// =========================================================
	// 通用存档读写（原有接口保留）
	// =========================================================

	UFUNCTION(BlueprintCallable, Category = "SaveGame")
	UYogSaveGame* GetCurrentSave();

	// 同步写盘（兼容旧代码，内部改为调用 DoAsyncSave）
	UFUNCTION(BlueprintCallable, Category = "SaveGame")
	void WriteSaveGame();

	UFUNCTION()
	void LoadSaveGame(UYogSaveGame* SaveGame);

	UPROPERTY(BlueprintAssignable)
	FOnSaveGameSignature OnSaveGameLoaded;

	UPROPERTY(BlueprintAssignable)
	FOnSaveGameSignature OnSaveGameWritten;

	UFUNCTION()
	void OnLevelLoaded(UWorld* LoadedWorld);

	UFUNCTION(BlueprintCallable)
	UYogSaveGame* CreateSaveGameInst();

	UFUNCTION()
	void SaveData(UObject* Object, UPARAM(ref) TArray<uint8>& Data);

	UFUNCTION()
	void LoadData(UObject* Object, UPARAM(ref) TArray<uint8>& Data);

	// 原有的 Player / Map 保存（保留供旧代码调用）
	UFUNCTION(BlueprintCallable, Category = "SaveGame")
	void SavePlayer(UYogSaveGame* SaveGame);

	UFUNCTION(BlueprintCallable, Category = "SaveGame")
	void LoadPlayer(UYogSaveGame* SaveGame);

	UFUNCTION(BlueprintCallable, Category = "SaveGame")
	void LoadMap(UYogSaveGame* SaveGame);

	UFUNCTION(BlueprintCallable, Category = "SaveGame")
	void SaveMap(UYogSaveGame* SaveGame);

	UFUNCTION(BlueprintCallable)
	FAbilitySaveData ConvertAbilitySpecToSaveData(const FGameplayAbilitySpec& Spec);

	UFUNCTION(BlueprintCallable)
	static FGameplayAbilitySpecHandle ConvertSaveDataToAbilitySpec(UYogAbilitySystemComponent* ASC, const FAbilitySaveData& SaveData);

	UFUNCTION(BlueprintCallable)
	void GiveAbilitiesFromSaveData(UYogAbilitySystemComponent* ASC, const TArray<FAbilitySaveData>& AbilitiesData);

	UPROPERTY()
	TObjectPtr<UYogSaveGame> CurrentSaveGame;

	// 当前激活的槽位索引（0-2），由 SelectSlot 设置
	UPROPERTY(BlueprintReadOnly, Category = "SaveGame|Slot")
	int32 CurrentSlotIndex = 0;

	// 触发一次异步写盘（内部有并发保护，可从外部调用）
	void DoAsyncSave();

	// Continue 按钮调用：若当前存档的 RunCheckpoint.bIsValid 则恢复到 GI->PendingRunState，返回是否成功
	UFUNCTION(BlueprintCallable, Category = "SaveGame|Checkpoint")
	bool TryRestoreRunCheckpoint();

	// =========================================================
	// 统计写入（各事件钩子调用，累积至 Statistics）
	// =========================================================

	// 新局开始时调用（StartNewRunFromFrontend）
	void RecordRunStarted();

	// 每次击杀 Count 个敌人时调用（UpdateFinishLevel）
	void RecordEnemyKilled(int32 Count);

	// 玩家死亡时调用（HandlePlayerDeath）
	void RecordPlayerDeath();

	// 金币增加时调用（BackpackGridComponent::AddGold）
	void RecordGoldEarned(int32 Amount);

private:

	// 防止并发异步写盘的标志
	bool bAsyncSavePending = false;

	// pending 期间又触发了写盘请求，完成后补一次
	bool bSaveDirtyWhilePending = false;

	// 本局开始时间（用于计算 TotalPlayTimeSeconds）
	FDateTime RunStartTime;

	UPROPERTY()
	TObjectPtr<UYogSettingsSave> CurrentSettings;

	// 核心异步写盘（由 TriggerCheckpoint / QuickSave / WriteSaveGame 调用）

	// FRunState ↔ FRunCheckpointData 互转
	void PopulateCheckpointFromRunState(FRunCheckpointData& OutCheckpoint, int32 Floor);
	void RestoreRunStateFromCheckpoint(const FRunCheckpointData& InCheckpoint);

	void LoadLevelData(UYogSaveGame* SaveGame);

	// 异步写盘完成回调
	UFUNCTION()
	void OnAsyncSaveComplete(const FString& SlotName, const int32 UserIndex, bool bSuccess);

	// 存档版本迁移（顺序逐版本升级）
	void MigrateSaveGame(UYogSaveGame* Save, int32 FromVersion, int32 ToVersion);
};


//UObject* loadObj = StaticLoadObject(UBlueprint::StaticClass(), NULL, TEXT("Blueprint'/Game/Code/Characters/B_PlayerBase.B_PlayerBase_C'"));
//if (loadObj != nullptr)
//{
//	PlayerBlueprintClass = loadObj->GetClass();
//	//UBlueprint* ubp = Cast<UBlueprint>(loadObj);
//	//AActor* spawnActor = GetWorld()->SpawnActor<AActor>(ubp->GeneratedClass);
//	//UE_LOG(LogClass, Log, TEXT("Success"));
//}
