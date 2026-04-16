// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "ModularPawn.h"

#include "YogCameraPawn.generated.h"

class AYogPlayerControllerBase;
class AYogCharacterBase;
class AYogGameMode;
class ACameraConstraintActor;
class UCameraShakeBase;
class UFloatingPawnMovement;
class AAIController;

// ─────────────────────────────────────────────────────────────────────────────
// 相机状态枚举
// ─────────────────────────────────────────────────────────────────────────────

/**
 * 相机当前的主要驱动状态（优先级由高到低）：
 *   Dash        → 冲刺：1:1 无延迟跟随玩家
 *   CombatFocus → 战斗中，搜索半径内有敌人：向敌人质心偏移
 *   CombatSearch→ 战斗中，搜索半径内无敌人：向最近敌人方向偏移
 *   PickupFocus → 整理阶段，场景中有拾取物：向拾取物偏移
 *   LookAhead   → 玩家移动中：逐步向移动方向前方偏移
 *   FocusCharacter → 玩家静止：缓慢回到玩家中心
 *   Idle        → 未使用（预留）
 */
UENUM(BlueprintType)
enum class EYogCameraStates : uint8
{
	FocusCharacter	UMETA(DisplayName = "FocusCharacter"),
	LookAhead		UMETA(DisplayName = "LookAhead"),
	Dash			UMETA(DisplayName = "Dash"),
	CombatFocus		UMETA(DisplayName = "CombatFocus"),
	CombatSearch	UMETA(DisplayName = "CombatSearch"),
	PickupFocus		UMETA(DisplayName = "PickupFocus"),
	Idle			UMETA(DisplayName = "Idle")
};

// ─────────────────────────────────────────────────────────────────────────────
// AYogCameraPawn
// ─────────────────────────────────────────────────────────────────────────────

/**
 * AYogCameraPawn — 游戏主相机 Pawn
 *
 * 作为独立 Pawn 控制相机位置，通过 SetViewTarget 指定为玩家相机。
 * Owner 设置为玩家角色，每帧跟随其位置并叠加各类偏移。
 *
 * 功能概述：
 *   1. 静止时缓慢 Focus 到玩家中心
 *   2. 移动时先产生自然落后感，持续移动后逐步 Look-Ahead
 *   3. 冲刺时 1:1 无延迟跟随（无落后）
 *   4. 多边形边界约束（放 ACameraConstraintActor 到场景即可）
 *   5. 战斗时向敌人质心偏移；整理阶段向拾取物偏移
 *   6. 手柄右摇杆 / 鼠标位置驱动细微偏移
 *   7. 特殊事件（重伤、暴击）触发可配置的相机震动
 */
UCLASS(BlueprintType, Blueprintable)
class DEVKIT_API AYogCameraPawn : public AModularPawn
{
	GENERATED_BODY()

public:
	AYogCameraPawn(const FObjectInitializer& ObjectInitializer);

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void PossessedBy(AController* NewController) override;

	// ─── 组件 ─────────────────────────────────────────────────────────────

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	TObjectPtr<UFloatingPawnMovement> FloatingMovementComponent;

	/** AI Controller（相机 Pawn 使用 AIController 移动） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<AAIController> CameraController;

	// ─── 状态 ─────────────────────────────────────────────────────────────

	UPROPERTY(BlueprintReadOnly, Category = "Camera|State")
	EYogCameraStates CameraStatus = EYogCameraStates::FocusCharacter;

	UPROPERTY(BlueprintReadOnly, Category = "Camera|State")
	EYogCameraStates PrevStatus = EYogCameraStates::FocusCharacter;

	/** 修改相机状态（相同状态则忽略，避免重复触发回调） */
	UFUNCTION(BlueprintCallable, Category = "Camera|State")
	void SetCameraStates(EYogCameraStates NewState);

	/** 状态变化时调用，可在蓝图中覆盖实现过渡效果 */
	UFUNCTION(BlueprintCallable, Category = "Camera|State")
	void OnCameraStatesChanged(EYogCameraStates PreviousState, EYogCameraStates NextState);

	// ─── 前瞻（LookAhead）参数 ────────────────────────────────────────────

	/** 玩家持续移动多少秒后达到最大前瞻偏移（默认 0.5s） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|LookAhead",
		meta = (ClampMin = "0.1"))
	float LookAheadBuildupTime = 0.5f;

	/** 最大前瞻偏移距离（世界单位） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|LookAhead",
		meta = (ClampMin = "0.0"))
	float LookAheadDistance = 280.0f;

	/** 前瞻状态下相机跟随速度（数值越大跟随越紧） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|LookAhead",
		meta = (ClampMin = "0.1"))
	float LookAheadLerpSpeed = 4.0f;

	/** 刚起步时（Alpha=0）相机跟随速度，比 LookAheadLerpSpeed 慢，产生初期落后感 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|LookAhead",
		meta = (ClampMin = "0.1"))
	float InitialFollowLerpSpeed = 2.0f;

	/** 前瞻 Alpha 衰减速度（玩家停止移动后 Alpha 归零的速率） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|LookAhead",
		meta = (ClampMin = "0.1"))
	float LookAheadAlphaDecaySpeed = 5.0f;

	// ─── 静止聚焦（FocusCharacter）参数 ──────────────────────────────────

	/** 玩家静止时相机回归中心的速度（慢于跟随速度，产生自然聚焦感） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Focus",
		meta = (ClampMin = "0.1"))
	float FocusLerpSpeed = 1.5f;

	// ─── 战斗感知（Combat）参数 ───────────────────────────────────────────

	/** 判定为"附近有敌人"的搜索半径（世界单位） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Combat",
		meta = (ClampMin = "100.0"))
	float CombatSearchRadius = 1200.0f;

	/** CombatFocus 状态：向敌人质心偏移的最大距离 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Combat",
		meta = (ClampMin = "0.0"))
	float CombatBiasDistance = 220.0f;

	/** CombatSearch 状态（敌人不在搜索半径内）：向最近敌人方向偏移的距离 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Combat",
		meta = (ClampMin = "0.0"))
	float CombatSearchOffset = 160.0f;

	/** 战斗状态下相机跟随偏移的速度 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Combat",
		meta = (ClampMin = "0.1"))
	float CombatLerpSpeed = 2.5f;

	// ─── 拾取物聚焦（PickupFocus）参数 ───────────────────────────────────

	/** 整理阶段向拾取物偏移的最大距离 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Pickup",
		meta = (ClampMin = "0.0"))
	float PickupBiasDistance = 150.0f;

	/** 拾取物聚焦状态的相机跟随速度 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Pickup",
		meta = (ClampMin = "0.1"))
	float PickupLerpSpeed = 2.0f;

	// ─── 输入偏移（手柄/鼠标）参数 ───────────────────────────────────────

	/** 手柄右摇杆 / 鼠标引起的最大相机偏移（世界单位） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Input",
		meta = (ClampMin = "0.0"))
	float MaxInputOffset = 200.0f;

	/** 输入偏移平滑追踪速度 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Input",
		meta = (ClampMin = "0.1"))
	float InputOffsetLerpSpeed = 8.0f;

	/**
	 * 是否自动从鼠标位置读取偏移。
	 * 鼠标偏离屏幕中心时相机随之微偏；手柄右摇杆激活（|RawInputAxis| > 0.1）时鼠标读取暂停。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Input")
	bool bAutoReadMouseOffset = true;

	// ─── 冲刺（Dash）参数 ─────────────────────────────────────────────────

	/** 当前是否处于冲刺状态（由 GA_Dash 或 PlayerController 调用 SetDashMode 设置） */
	UPROPERTY(BlueprintReadOnly, Category = "Camera|Dash")
	bool bIsDashing = false;

	// ─── 移动判定 ─────────────────────────────────────────────────────────

	/** 速度超过此值视为"正在移动"（世界单位/秒，XY 平面） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Movement",
		meta = (ClampMin = "1.0"))
	float MovingSpeedThreshold = 10.0f;

	// ─── 相机震动事件 ──────────────────────────────────────────────────────

	/** 普通重伤时触发的相机震动（在 Blueprint Details 中指定） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Events")
	TSubclassOf<UCameraShakeBase> HeavyHitShakeClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Events",
		meta = (ClampMin = "0.0"))
	float HeavyHitShakeScale = 1.0f;

	/** 暴击时触发的相机震动 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Events")
	TSubclassOf<UCameraShakeBase> CritHitShakeClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Events",
		meta = (ClampMin = "0.0"))
	float CritHitShakeScale = 1.0f;

	// ─── 公开接口 ─────────────────────────────────────────────────────────

	/**
	 * 进入/退出冲刺状态。
	 * 由 GA_Dash 激活时调用 true，结束时调用 false。
	 */
	UFUNCTION(BlueprintCallable, Category = "Camera|Dash")
	void SetDashMode(bool bDashing);

	/**
	 * 设置输入轴偏移（手柄右摇杆或归一化鼠标位置）。
	 * Axis 为 [-1, 1] 范围的归一化向量，相机会按 MaxInputOffset 缩放。
	 */
	UFUNCTION(BlueprintCallable, Category = "Camera|Input")
	void SetCameraInputAxis(FVector2D Axis);

	/** 触发重伤相机震动（被暴击等强击事件时调用） */
	UFUNCTION(BlueprintCallable, Category = "Camera|Events")
	void NotifyHeavyHit();

	/** 触发暴击相机震动 */
	UFUNCTION(BlueprintCallable, Category = "Camera|Events")
	void NotifyCritHit();

	// ─── 旧接口兼容（供蓝图使用） ─────────────────────────────────────────

	/** @deprecated 请改用 SetDashMode。直接设置状态（兼容旧蓝图）。 */
	UFUNCTION(BlueprintCallable, Category = "Camera|State")
	void SetCameraStatesLegacy(EYogCameraStates NewMovementMode) { SetCameraStates(NewMovementMode); }

	/** VolumeOverlapLoc 保留供旧代码使用，新系统使用 ACameraConstraintActor */
	UPROPERTY(BlueprintReadOnly, Category = "Camera|Legacy")
	FVector VolumeOverlapLoc;

	UFUNCTION(BlueprintCallable, Category = "Camera|Legacy")
	void SetVolumeOverlapLoc(FVector Loc) { VolumeOverlapLoc = Loc; }

protected:
	// ─── 内部状态（每帧更新） ─────────────────────────────────────────────

	/** 玩家持续移动的累计时间（停止即重置） */
	float MovingTime = 0.0f;

	/** 当前前瞻 Alpha [0,1]：0 = 无前瞻, 1 = 满前瞻 */
	float CurrentLookAheadAlpha = 0.0f;

	/** 最近一次非零移动方向（世界空间 XY 平面单位向量） */
	FVector LastMovementDir = FVector::ForwardVector;

	/** 当前输入偏移（世界空间，经过平滑插值） */
	FVector CurrentInputOffset = FVector::ZeroVector;

	/**
	 * 手柄右摇杆原始轴值（[-1,1]，由 SetCameraInputAxis 设置）。
	 * 手柄松开时需由调用方传入 ZeroVector（或通过 ETriggerEvent::Completed 绑定）。
	 */
	FVector2D GamepadInputAxis = FVector2D::ZeroVector;

	/** 缓存的 ConstraintActor（场景内第一个，BeginPlay 时查找） */
	TWeakObjectPtr<ACameraConstraintActor> CachedConstraintActor;

	// ─── 主逻辑函数 ───────────────────────────────────────────────────────

	/** 每帧主更新入口 */
	void UpdateCameraLoc(float DeltaTime);

	/** 根据优先级决定当前状态 */
	void DetermineState(const FVector& PlayerPos, bool bIsMoving);

	/** 根据当前状态计算 XY 平面偏移量 */
	FVector2D ComputeStateOffset(const FVector& PlayerPos) const;

	/** 获取当前状态对应的 Lerp 速度 */
	float GetCurrentLerpSpeed() const;

	/** 查找场景中最近的 ARewardPickup（arrangement 阶段使用） */
	AActor* FindNearestPickup(const FVector& PlayerPos) const;

	/** 获取 CachedConstraintActor（懒加载，只在 BeginPlay + 失效时查找） */
	ACameraConstraintActor* GetConstraintActor();

	/** 获取 GameMode（每次直接 Cast，开销极小） */
	AYogGameMode* GetGameMode() const;

private:
	TObjectPtr<AYogPlayerControllerBase> PlayerController;
};
