// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "BehaviorTree/BehaviorTree.h"
#include "GameplayTagContainer.h"
#include "AbilitySystem/Abilities/YogGameplayAbility.h"
#include "Data/CharacterData.h"
#include "NiagaraSystem.h"
#include "EnemyData.generated.h"

class AEnemyCharacterBase;
class AActor;
class URuneDataAsset;

// =========================================================
// Buff 条目（携带难度扣分）
//
// 用于两个场景：
//   1. URoomDataAsset.BuffPool —— 关卡 Buff，刷怪时对每只怪额外扣分
//   2. UEnemyData.EnemyBuffPool —— 敌人专属 Buff，BuildWavePlan 选取时扣分
// =========================================================

USTRUCT(BlueprintType)
struct DEVKIT_API FBuffEntry
{
	GENERATED_BODY()

	// 关联的符文/Buff 数据资产
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Buff")
	TObjectPtr<URuneDataAsset> RuneDA;

	// 将此 Buff 施加给敌人时，从波次预算中额外扣除的难度分
	// （代表该 Buff 使敌人更强，因此占用更多预算）
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Buff", meta = (ClampMin = "0"))
	int32 DifficultyScore = 1;

	// 实际施加概率。1 = 必定施加，0 = 永不施加。
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Buff", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ApplyChance = 1.0f;
};

UENUM(BlueprintType)
enum class EEnemyAIApproachStyle : uint8
{
	Direct      UMETA(DisplayName = "Direct"),
	SwarmFlank  UMETA(DisplayName = "Swarm Flank"),
	BruiserHold UMETA(DisplayName = "Bruiser Hold"),
};

UENUM(BlueprintType)
enum class EEnemyAIState : uint8
{
	Patrol UMETA(DisplayName = "Patrol"),
	Alert  UMETA(DisplayName = "Alert"),
	Combat UMETA(DisplayName = "Combat"),
};

UENUM(BlueprintType)
enum class EEnemyAIAttackMovementMode : uint8
{
	None        UMETA(DisplayName = "None"),
	RadialLunge UMETA(DisplayName = "Radial Lunge"),
};

UENUM(BlueprintType)
enum class EEnemyAIAttackRole : uint8
{
	CloseMelee      UMETA(DisplayName = "Close Melee"),
	SpecialMovement UMETA(DisplayName = "Special Movement"),
	Skill           UMETA(DisplayName = "Skill"),
};

USTRUCT(BlueprintType)
struct DEVKIT_API FEnemyAIMovementTuning
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Movement")
	EEnemyAIApproachStyle ApproachStyle = EEnemyAIApproachStyle::Direct;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Movement", meta = (ClampMin = "0.0"))
	float PreferredRange = 220.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Movement", meta = (ClampMin = "0.0"))
	float AttackRange = 180.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Movement", meta = (ClampMin = "0.0"))
	float AcceptanceRadius = 70.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Movement", meta = (ClampMin = "0.05"))
	float RepathInterval = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Movement", meta = (ClampMin = "0.0"))
	float FlankDistance = 160.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Movement", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float StrafeChance = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Movement", meta = (ClampMin = "0.0"))
	float CrowdSeparationWeight = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Movement|Forward Steering")
	bool bUseForwardSteering = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Movement|Forward Steering", meta = (ClampMin = "0.0"))
	float ForwardTurnLeadDistance = 220.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Movement|Forward Steering", meta = (ClampMin = "0.0"))
	float MaxTurnYawSpeed = 360.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Movement|Forward Steering", meta = (ClampMin = "0.0"))
	float MoveTargetSmoothingSpeed = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Movement|Forward Steering", meta = (ClampMin = "0.0", ClampMax = "180.0"))
	float SharpTurnAngle = 120.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Movement", meta = (ClampMin = "0.0"))
	float MaxWalkSpeedOverride = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Movement|Combat Slot", meta = (ClampMin = "0.0"))
	float CombatSlotLockDuration = 1.2f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Movement|Combat Slot", meta = (ClampMin = "0.0"))
	float AttackRangeExitBuffer = 40.f;
};

USTRUCT(BlueprintType)
struct DEVKIT_API FEnemyAIAwarenessTuning
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Awareness", meta = (ClampMin = "0.0"))
	float DetectionRadius = 900.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Awareness", meta = (ClampMin = "0.0"))
	float CombatEnterRadius = 650.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Awareness", meta = (ClampMin = "0.0"))
	float CombatExitRadius = 1200.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Awareness", meta = (ClampMin = "0.0"))
	float LoseTargetDelay = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Awareness", meta = (ClampMin = "0.0"))
	float AlertDuration = 4.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Awareness", meta = (ClampMin = "0.0"))
	float AlertBroadcastRadius = 1200.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Awareness", meta = (ClampMin = "0.0"))
	float PatrolRadius = 600.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Awareness", meta = (ClampMin = "0.0"))
	float PatrolWaitMin = 0.6f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Awareness", meta = (ClampMin = "0.0"))
	float PatrolWaitMax = 1.5f;
};

USTRUCT(BlueprintType)
struct DEVKIT_API FEnemyAIAttackOption
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Attack")
	FName AttackName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Attack")
	FGameplayTagContainer AbilityTags;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Attack", meta = (ClampMin = "0.0"))
	float MinRange = 0.0f;

	// 0 means no upper range limit.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Attack", meta = (ClampMin = "0.0"))
	float MaxRange = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Attack", meta = (ClampMin = "0.0"))
	float Weight = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Attack", meta = (ClampMin = "0.0"))
	float Cooldown = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Attack")
	bool bPreAttackFlash = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Attack")
	EEnemyAIAttackRole AttackRole = EEnemyAIAttackRole::CloseMelee;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Attack|Movement")
	EEnemyAIAttackMovementMode AttackMovementMode = EEnemyAIAttackMovementMode::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Attack|Movement", meta = (ClampMin = "0.0"))
	float LungeStartRange = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Attack|Movement", meta = (ClampMin = "0.0"))
	float LungeDistance = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Attack|Movement", meta = (ClampMin = "0.0"))
	float LungeDuration = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Attack|Movement", meta = (ClampMin = "0.0"))
	float LungeStopDistance = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Attack|Movement", meta = (ClampMin = "0.0"))
	float MovementAttackRangeMultiplier = 2.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Attack|Movement", meta = (ClampMin = "0.0"))
	float MovementAttackCooldown = 10.0f;
};

USTRUCT(BlueprintType)
struct DEVKIT_API FEnemyAIAttackProfile
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Attack")
	TArray<FEnemyAIAttackOption> Attacks;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Attack|Selection", meta = (ClampMin = "0.0"))
	float RecentAttackMemoryDuration = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Attack|Selection", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float RepeatAttackWeightMultiplier = 0.25f;
};

USTRUCT(BlueprintType)
struct DEVKIT_API FEnemyAIAttackRuntimeContext
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI|Attack")
	bool bValid = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI|Attack")
	FEnemyAIAttackOption AttackOption;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI|Attack")
	TObjectPtr<AActor> TargetActor = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI|Attack")
	float DistanceToTarget = 0.f;
};


USTRUCT(BlueprintType)
struct FEnemyAbilityData : public FTableRowBase
{
	GENERATED_BODY()

public:


	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<UYogGameplayAbility> MobAbility;

};


UCLASS()
class DEVKIT_API UEnemyData : public UCharacterData
{
	GENERATED_BODY()

public:

	// 对应的敌人 Actor 类（刷怪系统用此类生成敌人）
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy")
	TSubclassOf<AEnemyCharacterBase> EnemyClass;

	// 难度预算消耗（普通怪建议 2-4，精英怪建议 6-10）
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy")
	int32 DifficultyScore = 3;

	// 此敌人专属的 Buff 池（如死亡守卫概率护甲）
	// BuildWavePlan 时逐条按 ApplyChance 判定；生效条目的 DifficultyScore 会从波次预算中额外扣除
	// 若为空，则不选取敌人专属 Buff
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Buff")
	TArray<FBuffEntry> EnemyBuffPool;

	// 此敌人使用的行为树（留空则使用 AIController 默认行为树）
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|AI")
	TObjectPtr<UBehaviorTree> BehaviorTree;

	// YogAIController 使用的移动、寻路和避让调参。
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|AI")
	FEnemyAIMovementTuning MovementTuning;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|AI")
	FEnemyAIAwarenessTuning AwarenessTuning;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|AI")
	FEnemyAIAttackProfile AttackProfile;

	// 预生成特效（留空 = 无 FX，立即刷出）
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Spawn")
	TObjectPtr<UNiagaraSystem> PreSpawnFX;

	// 预生成特效持续时间（秒）。FX 结束后才真正 SpawnActor；0 = 立即刷出
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Spawn")
	float PreSpawnFXDuration = 0.f;

	// 连续被击中多少次后进入霸体（0 = 永不触发霸体）
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Poise", meta = (ClampMin = "0"))
	int32 SuperArmorThreshold = 3;

	// 霸体持续时间（秒）
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Poise", meta = (ClampMin = "0.0"))
	float SuperArmorDuration = 2.f;
};
