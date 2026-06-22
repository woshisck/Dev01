#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "BuffFlowTypes.generated.h"

class AEnemyCharacterBase;
class AMobSpawner;
class AYogCharacterBase;
class UEnemyData;
class UEnemyWeaponDefinition;
class URuneDataAsset;

/**
 * GE 堆叠模式
 *   None      — 每次命中独立创建一个 GE 实例（无去重）
 *   Unique    — 同目标只保留一个实例，重复命中刷新持续时间
 *   Stackable — 同目标共享一个实例，可叠加多层（配合 Max Stacks 使用）
 */
UENUM(BlueprintType)
enum class EBFGEStackMode : uint8
{
	None      UMETA(DisplayName = "None"),
	Unique    UMETA(DisplayName = "Unique"),
	Stackable UMETA(DisplayName = "Stackable"),
};

/**
 * 目标选择器 —— BuffFlow 节点中选择作用目标
 */
UENUM(BlueprintType)
enum class EBFTargetSelector : uint8
{
	LifecycleTarget  UMETA(DisplayName = "Lifecycle Target"),
	BuffOwner        UMETA(DisplayName = "Buff拥有者"),
	BuffGiver        UMETA(DisplayName = "Buff施加者"),
	LastDamageTarget UMETA(DisplayName = "上次伤害目标"),   // DamageReceiver（被击者）
	DamageCauser     UMETA(DisplayName = "伤害来源"),       // DamageCauser（攻击者）
	AllHitTargets    UMETA(DisplayName = "所有命中目标"),   // all actors from DamageReceivers (anim notify multi-hit)
};

/**
 * 事件上下文 —— 由触发器节点写入，供后续节点读取
 * 统一替代之前分散的 LastDamageTargetActor 字段
 */
UENUM(BlueprintType)
enum class EBuffFlowLifecycleType : uint8
{
	None  UMETA(DisplayName = "None"),
	Spawn UMETA(DisplayName = "Spawn"),
	Death UMETA(DisplayName = "Death"),
};

USTRUCT(BlueprintType)
struct DEVKIT_API FBuffFlowLifecycleContext
{
	GENERATED_BODY()

	UPROPERTY()
	EBuffFlowLifecycleType Type = EBuffFlowLifecycleType::None;

	UPROPERTY()
	TWeakObjectPtr<AActor> LifecycleTarget;

	UPROPERTY()
	TWeakObjectPtr<AMobSpawner> Spawner;

	UPROPERTY()
	TWeakObjectPtr<UEnemyData> EnemyData;

	UPROPERTY()
	TWeakObjectPtr<UEnemyWeaponDefinition> EnemyWeaponDefinition;

	UPROPERTY()
	TSubclassOf<AEnemyCharacterBase> EnemyClass;

	UPROPERTY()
	FTransform SpawnTransform = FTransform::Identity;

	UPROPERTY()
	TArray<TObjectPtr<URuneDataAsset>> EnemyBuffs;

	UPROPERTY()
	int32 WaveIndex = INDEX_NONE;

	UPROPERTY()
	bool bSpawnFinalized = false;

	UPROPERTY()
	bool bStorySpawn = false;

	UPROPERTY()
	TWeakObjectPtr<AEnemyCharacterBase> SpawnedEnemy;

	UPROPERTY()
	TWeakObjectPtr<AYogCharacterBase> DyingCharacter;

	UPROPERTY()
	TWeakObjectPtr<AActor> Instigator;

	UPROPERTY()
	TWeakObjectPtr<AActor> Killer;

	UPROPERTY()
	FVector DeathLocation = FVector::ZeroVector;

	UPROPERTY()
	bool bFinishRequested = false;

	void Reset()
	{
		Type = EBuffFlowLifecycleType::None;
		LifecycleTarget.Reset();
		Spawner.Reset();
		EnemyData.Reset();
		EnemyWeaponDefinition.Reset();
		EnemyClass = nullptr;
		SpawnTransform = FTransform::Identity;
		EnemyBuffs.Reset();
		WaveIndex = INDEX_NONE;
		bSpawnFinalized = false;
		bStorySpawn = false;
		SpawnedEnemy.Reset();
		DyingCharacter.Reset();
		Instigator.Reset();
		Killer.Reset();
		DeathLocation = FVector::ZeroVector;
		bFinishRequested = false;
	}
};

USTRUCT(BlueprintType)
struct FBFEventContext
{
	GENERATED_BODY()

	/** 伤害来源（攻击者） */
	UPROPERTY()
	TWeakObjectPtr<AActor> DamageCauser;

	/** 受到伤害的人（被击者） */
	UPROPERTY()
	TWeakObjectPtr<AActor> DamageReceiver;

	/** 所有被命中的目标（AnimNotify 多目标命中时填写，供 AllHitTargets 选择器使用） */
	UPROPERTY()
	TArray<TWeakObjectPtr<AActor>> DamageReceivers;

	/** 伤害量 */
	UPROPERTY()
	float DamageAmount = 0.f;

	UPROPERTY()
	FVector AttackDirection = FVector::ZeroVector;

	/** 供 OnBuffAdded / OnBuffRemoved 节点使用的 Tag */
	UPROPERTY()
	FGameplayTag EventTag;

	void Reset()
	{
		DamageCauser.Reset();
		DamageReceiver.Reset();
		DamageReceivers.Reset();
		DamageAmount = 0.f;
		AttackDirection = FVector::ZeroVector;
		EventTag = FGameplayTag::EmptyTag;
	}
};

/**
 * 数学运算类型 —— 供 BFNode_MathFloat / BFNode_MathInt 使用
 */
UENUM(BlueprintType)
enum class EBFMathOp : uint8
{
	Add      UMETA(DisplayName = "+"),
	Subtract UMETA(DisplayName = "-"),
	Multiply UMETA(DisplayName = "×"),
	Divide   UMETA(DisplayName = "÷"),
};

/**
 * GE 移除模式 —— 供 BFNode_ApplyEffect 的 Remove 引脚使用
 *   AllStacks   — 移除所有堆叠层（整个 GE 消失）
 *   OneStack    — 只移除 1 层
 *   CustomCount — 移除指定层数（由 StacksToRemove 数据引脚填入）
 */
UENUM(BlueprintType)
enum class EBFRemoveMode : uint8
{
	AllStacks   UMETA(DisplayName = "All Stacks"),
	OneStack    UMETA(DisplayName = "One Stack"),
	CustomCount UMETA(DisplayName = "Custom Count"),
};
