// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "DevKit.h"
#include "GameFramework/SaveGame.h"
#include "System/YogGameInstanceBase.h"
#include "Character/PlayerCharacterBase.h"
#include "Item/Weapon/WeaponInstance.h"
#include "Animation/YogAnimInstance.h"
#include "AbilitySystem/Attribute/BaseAttributeSet.h"
#include "Item/Weapon/WeaponInstance.h"
#include "Tutorial/TutorialHintDataAsset.h"
#include "MetaProgression/MetaTypes.h"
#include "Story/StoryRuleTypes.h"
#include "YogSaveGame.generated.h"


class AYogCharacterBase;
class UYogGameInstanceBase;
class UYogGameplayEffect;
class UActiveSkillDataAsset;

class AYogCharacterBase;



USTRUCT()
struct DEVKIT_API FAttributeSaveData
{
	GENERATED_BODY()

public:

	FAttributeSaveData()
	{

		Attack = 0.0;
		AttackPower = 0.0;
		Health = 0.0;
		MaxHealth = 0.0;
		AttackSpeed = 0.0;
		AttackRange = 0.0;
		Sanity = 0.0;
		MoveSpeed = 0.0;
		Dodge = 0.0;
		Resilience = 0.0;
		Resist = 0.0;
		DmgTaken = 0.0;
		Crit_Rate = 0.0;
		Crit_Damage = 0.0;
	}

	UPROPERTY()
	float Attack;

	UPROPERTY()
	float AttackPower;

	UPROPERTY()
	float Health;

	UPROPERTY()
	float MaxHealth;

	UPROPERTY()
	float AttackSpeed;

	UPROPERTY()
	float AttackRange;

	UPROPERTY()
	float Sanity;

	UPROPERTY()
	float MoveSpeed;

	UPROPERTY()
	float Dodge;

	UPROPERTY()
	float Resilience;

	UPROPERTY()
	float Resist;

	UPROPERTY()
	float DmgTaken;

	UPROPERTY()
	float Crit_Rate;

	UPROPERTY()
	float Crit_Damage;

};


//TEST ONLY, SAVE FOR DELETE --START

USTRUCT(BlueprintType)
struct FWeaponInstanceData
{

	GENERATED_BODY()

public:
	// Identifier to find the actor in the world or know its class to spawn
	UPROPERTY()
	FString ActorClassPath; 
	
	UPROPERTY()
	FTransform Transform;

	UPROPERTY()
	FName AttachSocket;

	UPROPERTY()
	TSubclassOf<UYogAnimInstance> WeaponLayer;



	UPROPERTY()
	TSoftClassPtr<UAnimInstance> SavedAnimBlueprintSoftClass;

	UPROPERTY()
	FString WeaponLayerClassPath;


	/* Contains all 'SaveGame' marked variables of the Actor */
	UPROPERTY()
	TArray<uint8> ByteData;

};


USTRUCT(BlueprintType)
struct FWeaponMeshData
{
	GENERATED_BODY()

	// Identifier to find the actor in the world or know its class to spawn
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<TSubclassOf<AWeaponInstance>> weaponInstanceClasses;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName AttachSocket;
};


USTRUCT()
struct DEVKIT_API FCharacterSaveData
{
	GENERATED_BODY()

public:

	FCharacterSaveData()
	{
		Credits = 0;
		PersonalRecordTime = 0.0f;
		PlayerLocation = FVector::ZeroVector;
		PlayerRotation = FRotator::ZeroRotator;
		bResumeAtTransform = true;
	}

	/* Player Id defined by the online sub system (such as Steam) converted to FString for simplicity  */
	UPROPERTY()
	FString PlayerID;

	UPROPERTY()
	int32 Credits;

	/* Longest survival time */
	UPROPERTY()
	float PersonalRecordTime;

	/* Location if player was alive during save */
	UPROPERTY()
	FVector PlayerLocation;

	/* Orientation if player was alive during save */
	UPROPERTY()
	FRotator PlayerRotation;

	/* We don't always want to restore location, and may just resume player at specific respawn point in world. */
	UPROPERTY()
	bool bResumeAtTransform;

	UPROPERTY()
	FAttributeSaveData AttributeSaveData;

	UPROPERTY()
	TArray<uint8> ByteData;

};

//Save for ability ? use in future
USTRUCT(BlueprintType)
struct DEVKIT_API FAbilitySaveData
{
	GENERATED_BODY()

	UPROPERTY()
	TSubclassOf<UYogGameplayAbility> AbilityClass;

	UPROPERTY()
	int32 Level = 0;

	UPROPERTY()
	int32 InputID = -1;

	UPROPERTY()
	FSoftClassPath AbilityClassPath;
};


USTRUCT()
struct DEVKIT_API FPlayerGameTagData
{
	GENERATED_BODY()

	UPROPERTY()
	uint8 count = 0;

	UPROPERTY()
	FGameplayTag gameplayTag;
};

USTRUCT()
struct DEVKIT_API FPlayerGASData
{
	GENERATED_BODY()
public:

	UPROPERTY()
	FAttributeSaveData PlayerAttributeData;

	UPROPERTY()
	TMap<FGameplayTag, int32> PlayerOwnedTags;


	//UPROPERTY()
	//FAbilitySystemComponentData ASC_Data;

	UPROPERTY()
	TArray<FAbilitySaveData> Abilities;


	UPROPERTY()
	UAbilityData* WeaponAbilities = nullptr;

	UPROPERTY()
	TArray<uint8> CharacterByteData;

	void SetupAttribute(UBaseAttributeSet& playerAttribute)
	{
		PlayerAttributeData.Attack = playerAttribute.GetAttack();
		PlayerAttributeData.AttackPower = playerAttribute.GetAttackPower();
		PlayerAttributeData.Health = playerAttribute.GetHealth();
		PlayerAttributeData.MaxHealth = playerAttribute.GetMaxHealth();
		PlayerAttributeData.AttackSpeed = playerAttribute.GetAttackSpeed();
		PlayerAttributeData.AttackRange = playerAttribute.GetAttackRange();
		PlayerAttributeData.Sanity = playerAttribute.GetSanity();
		PlayerAttributeData.MoveSpeed = playerAttribute.GetMoveSpeed();
		PlayerAttributeData.Dodge = playerAttribute.GetDodge();
		PlayerAttributeData.Resilience = playerAttribute.GetResilience();
		PlayerAttributeData.Resist = playerAttribute.GetResist();
		PlayerAttributeData.DmgTaken = playerAttribute.GetDmgTaken();
		PlayerAttributeData.Crit_Rate = playerAttribute.GetCrit_Rate();
		PlayerAttributeData.Crit_Damage = playerAttribute.GetCrit_Damage();

	}
};

USTRUCT()
struct FYogMapStateData
{
	GENERATED_BODY()

public:

	/* Location if player was alive during save */
	UPROPERTY()
	FName LevelName;

	UPROPERTY()
	TArray<uint8> MapByteData;

};


// ============================================================
//  存档槽位预览（轻量读取，选档界面用）
//  单一事实源：bHasPendingRun 仅在读取存档后由 RunCheckpoint.bIsValid 派生
// ============================================================
USTRUCT(BlueprintType)
struct DEVKIT_API FSlotPreviewData
{
	GENERATED_BODY()

	UPROPERTY() bool      bHasData            = false;
	UPROPERTY() FDateTime LastPlayTime;
	UPROPERTY() int32     HighestFloor        = 0;
	UPROPERTY() bool      bHasPendingRun      = false; // 派生自 RunCheckpoint.bIsValid
	UPROPERTY() bool      bFirstRunTutorialActive = false;
	UPROPERTY() bool      bFirstRunTutorialCompleted = false;
	UPROPERTY() int32     TotalPlayTimeSeconds = 0;
};

// ============================================================
//  局外成长数据（含打造卡牌，统一归属；新游戏时清空）
// ============================================================
USTRUCT()
struct DEVKIT_API FMetaProgressionData
{
	GENERATED_BODY()

	// 9种局外货币 + 神秘点：Tag → 数量
	UPROPERTY() TMap<FGameplayTag, int32> MetaCurrencies;

	// 局外升级树节点等级：RowName（DT_MetaUpgradeNodes）→ 当前等级
	UPROPERTY() TMap<FName, int32> NodeLevels;

	// 已解锁功能（故事引擎 / 教程系统通过 IsFeatureUnlocked 查询）
	UPROPERTY() TSet<FGameplayTag> UnlockedFeatures;

	// 神秘侧当前等级（神秘点花费后更新）
	UPROPERTY() int32 MysticSideLevel = 0;

	// 已打造的起始符文卡牌（局内 BeginPlay 时 AsyncLoad 后 Grant）
	UPROPERTY() TArray<FPrimaryAssetId> CraftedStarterRunes;

	// 已打造的武器终结技卡牌
	UPROPERTY() TArray<FPrimaryAssetId> CraftedWeaponFinisherCards;
};

// ============================================================
//  局内存档点快照（FRunState 的可序列化版本）
//  UObject 指针全部改为 TSoftObjectPtr，AsyncLoad 后恢复。
//  单一事实源：bIsValid = true 时主菜单显示"继续游戏"
// ============================================================
USTRUCT()
struct DEVKIT_API FRunCheckpointData
{
	GENERATED_BODY()

	UPROPERTY() bool  bIsValid         = false;
	UPROPERTY() int32 CheckpointFloor  = 0;

	// ── 玩家状态 ────────────────────────────────────────────────
	UPROPERTY() float CurrentHP    = 0.f;
	UPROPERTY() int32 CurrentGold  = 0;
	UPROPERTY() int32 CurrentPhase = 0;
	UPROPERTY() float CurrentHeat  = 0.f;

	// ── 战斗卡组配置 ────────────────────────────────────────────
	UPROPERTY() int32 CompletedCombatBattleCount          = 0;
	UPROPERTY() float CombatDeckShuffleCooldownDuration   = 1.f;
	UPROPERTY() int32 CombatDeckMaxActiveSequenceSize      = 0;

	// ── 背包与符文（FPlacedRune / FRuneInstance 内部 UObject* 由 UE 存档系统处理）──
	UPROPERTY() TArray<FPlacedRune>   PlacedRunes;
	UPROPERTY() TArray<FRuneInstance> PendingRunes;
	UPROPERTY() TArray<FRuneInstance> HiddenPassiveRuneInstances;
	UPROPERTY() TArray<FSacrificeOfferingCostState> SacrificeOfferingCosts;

	// ── 软引用（明确控制加载时机）──────────────────────────────
	UPROPERTY() TSoftObjectPtr<UWeaponDefinition> EquippedWeaponDef;
	UPROPERTY() TSoftObjectPtr<UWeaponDefinition> InactiveWeaponDef;
	UPROPERTY() TSoftObjectPtr<USacrificeGraceDA> ActiveSacrificeGrace;

	// 战斗卡组顺序（软引用列表，切关时保存玩家改动）
	UPROPERTY() TArray<TSoftObjectPtr<URuneDataAsset>> CombatDeckCards;
	UPROPERTY() TArray<ECombatCardLinkOrientation>     CombatDeckCardOrientations;
	UPROPERTY() TArray<TSoftObjectPtr<URuneDataAsset>> InactiveCombatDeckCards;
	UPROPERTY() TArray<ECombatCardLinkOrientation>     InactiveCombatDeckCardOrientations;
	UPROPERTY() float InactiveCombatDeckShuffleCooldownDuration = 1.f;
	UPROPERTY() int32 InactiveCombatDeckMaxActiveSequenceSize = 0;
	UPROPERTY() TArray<TSoftObjectPtr<UActiveSkillDataAsset>> SelectedSkillLoadout;
};

// ============================================================
//  跑局统计数据（新游戏/重置槽位时保留，跨局累积）
// ============================================================
USTRUCT(BlueprintType)
struct DEVKIT_API FGameStatistics
{
	GENERATED_BODY()

	UPROPERTY() int32 TotalRuns             = 0;
	UPROPERTY() int32 TotalKills            = 0;
	UPROPERTY() int32 HighestFloor          = 0;
	UPROPERTY() int32 TotalPlayTimeSeconds  = 0;
	UPROPERTY() int32 TotalDeaths           = 0;
	UPROPERTY() int32 TotalGoldEarned       = 0;
};

// ============================================================

UCLASS(BlueprintType)
class DEVKIT_API UYogSaveGame : public USaveGame
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadWrite)
	TSubclassOf<class APlayerCharacterBase> SavedCharacterClass;

	UPROPERTY()
	FPlayerGASData PlayerStateData;

	UPROPERTY()
	TArray<FWeaponInstanceData> WeaponInstanceItems;

	UPROPERTY()
	FYogMapStateData MapStateData;

	//ACTOR WITH GAS SYSTEM ATTACHED
	UPROPERTY()
	TArray<FCharacterSaveData> SavedCharacter;

	/* Actors stored from a level (currently does not support a specific level and just assumes the demo map) */

	// 教程引导状态（新存档默认 NeedWeaponTutorial，引导完成后 Completed）
	UPROPERTY()
	ETutorialState TutorialState = ETutorialState::NeedWeaponTutorial;

	UPROPERTY()
	int32 FirstRunTutorialStage = 0;

	// 已展示过的 Save-scope 一次性弹窗 key（UYogUIManagerSubsystem::PushScreenOnce 写入）。
	UPROPERTY()
	TSet<FGameplayTag> ShownPopupKeys;

	// Story Engine MVP：跨存档故事标记，供教程、首局引导、主城解锁共享。
	UPROPERTY()
	TMap<FGameplayTag, bool> StoryFlags;

	// Story Engine MVP：OncePerSave 规则触发记录。
	UPROPERTY()
	TSet<FName> StoryFiredRuleIds;

	// Story Engine MVP：遗圣目录/轻量任务状态。完整对话任务线后续再扩展。
	UPROPERTY()
	TMap<FGameplayTag, FStoryQuestTaskData> StoryQuestTasks;

	// ── 槽位元信息 ──────────────────────────────────────────────
	UPROPERTY() FDateTime SlotCreatedTime;
	UPROPERTY() FDateTime SlotLastPlayTime;
	UPROPERTY() int32     SaveFormatVersion = 1;

	// ── 局外成长（新游戏时清空，统计数据保留）──────────────────
	UPROPERTY() FMetaProgressionData MetaProgression;
	UPROPERTY() TArray<TSoftObjectPtr<UActiveSkillDataAsset>> SelectedSkillLoadout;

	// ── 存档点快照（退出时保留，死亡/结局后清除）───────────────
	UPROPERTY() FRunCheckpointData RunCheckpoint;

	// ── 统计数据（新游戏不清除，跨局累积）──────────────────────
	UPROPERTY() FGameStatistics Statistics;

	FCharacterSaveData* GetPlayerData(APlayerState* PlayerState);

};
