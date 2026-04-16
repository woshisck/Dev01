// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Camera/PlayerCameraManager.h"
#include "YogPlayerCameraManager.generated.h"

class AYogCameraVolume;
class AYogGameMode;
class UCameraShakeBase;

// ─────────────────────────────────────────────────────────────────────────────
// 相机状态枚举（优先级由高到低）
// ─────────────────────────────────────────────────────────────────────────────

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
// AYogPlayerCameraManager
// ─────────────────────────────────────────────────────────────────────────────

/**
 * AYogPlayerCameraManager
 *
 * 通过重写 UpdateViewTarget() 在 SpringArm + Camera 基础 POV 上叠加智能偏移。
 * 玩家角色蓝图中需挂载 UYogSpringArmComponent + UCameraComponent。
 *
 * 状态优先级（高→低）：
 *   Dash > CombatFocus > CombatSearch > PickupFocus > LookAhead > FocusCharacter
 */
UCLASS()
class DEVKIT_API AYogPlayerCameraManager : public APlayerCameraManager
{
	GENERATED_BODY()

public:
	AYogPlayerCameraManager(const FObjectInitializer& ObjectInitializer);

	// ─── 核心重写 ──────────────────────────────────────────────────────────

	virtual void UpdateViewTarget(FTViewTarget& OutVT, float DeltaTime) override;

	// ─── 公开 API ──────────────────────────────────────────────────────────

	/** 进入/退出冲刺模式（由 GA_PlayerDash 调用） */
	UFUNCTION(BlueprintCallable, Category = "Camera")
	void SetDashMode(bool bDashing);

	/** 设置手柄右摇杆偏移轴（[-1,1] 归一化，松开时传 ZeroVector） */
	UFUNCTION(BlueprintCallable, Category = "Camera")
	void SetCameraInputAxis(FVector2D Axis);

	/** 触发重伤相机震动 */
	UFUNCTION(BlueprintCallable, Category = "Camera")
	void NotifyHeavyHit();

	/** 触发暴击相机震动 */
	UFUNCTION(BlueprintCallable, Category = "Camera")
	void NotifyCritHit();

	/** 注册/注销当前有效边界 Volume（由 AYogCameraVolume Overlap 回调调用） */
	UFUNCTION(BlueprintCallable, Category = "Camera")
	void SetConstraintVolume(AYogCameraVolume* Volume);

	// ─── 状态（只读） ─────────────────────────────────────────────────────

	UPROPERTY(BlueprintReadOnly, Category = "Camera|State")
	EYogCameraStates CameraStatus = EYogCameraStates::FocusCharacter;

	UPROPERTY(BlueprintReadOnly, Category = "Camera|State")
	EYogCameraStates PrevStatus = EYogCameraStates::FocusCharacter;

	// ─── 前瞻（LookAhead）参数 ────────────────────────────────────────────

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|LookAhead", meta = (ClampMin = "0.1"))
	float LookAheadBuildupTime = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|LookAhead", meta = (ClampMin = "0.0"))
	float LookAheadDistance = 280.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|LookAhead", meta = (ClampMin = "0.1"))
	float LookAheadLerpSpeed = 4.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|LookAhead", meta = (ClampMin = "0.1"))
	float InitialFollowLerpSpeed = 2.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|LookAhead", meta = (ClampMin = "0.1"))
	float LookAheadAlphaDecaySpeed = 5.f;

	// ─── 静止聚焦（FocusCharacter）参数 ──────────────────────────────────

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Focus", meta = (ClampMin = "0.1"))
	float FocusLerpSpeed = 1.5f;

	// ─── 战斗感知（Combat）参数 ───────────────────────────────────────────

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Combat", meta = (ClampMin = "100.0"))
	float CombatSearchRadius = 1200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Combat", meta = (ClampMin = "0.0"))
	float CombatBiasDistance = 220.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Combat", meta = (ClampMin = "0.0"))
	float CombatSearchOffset = 160.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Combat", meta = (ClampMin = "0.1"))
	float CombatLerpSpeed = 2.5f;

	// ─── 拾取物聚焦（PickupFocus）参数 ───────────────────────────────────

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Pickup", meta = (ClampMin = "0.0"))
	float PickupBiasDistance = 150.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Pickup", meta = (ClampMin = "0.1"))
	float PickupLerpSpeed = 2.f;

	// ─── 输入偏移（手柄/鼠标）参数 ───────────────────────────────────────

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Input", meta = (ClampMin = "0.0"))
	float MaxInputOffset = 200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Input", meta = (ClampMin = "0.1"))
	float InputOffsetLerpSpeed = 8.f;

	/** 手柄未激活时是否自动读取鼠标位置作为偏移源 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Input")
	bool bAutoReadMouseOffset = true;

	// ─── 移动判定 ─────────────────────────────────────────────────────────

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Movement", meta = (ClampMin = "1.0"))
	float MovingSpeedThreshold = 10.f;

	// ─── 相机震动 ─────────────────────────────────────────────────────────

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Events")
	TSubclassOf<UCameraShakeBase> HeavyHitShakeClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Events", meta = (ClampMin = "0.0"))
	float HeavyHitShakeScale = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Events")
	TSubclassOf<UCameraShakeBase> CritHitShakeClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Events", meta = (ClampMin = "0.0"))
	float CritHitShakeScale = 1.f;

protected:
	virtual void DisplayDebug(UCanvas* Canvas, const FDebugDisplayInfo& DebugDisplay, float& YL, float& YPos) override;

private:
	// ─── 内部运行时状态 ───────────────────────────────────────────────────

	bool      bIsDashing             = false;
	float     MovingTime             = 0.f;
	float     CurrentLookAheadAlpha  = 0.f;
	FVector   LastMovementDir        = FVector::ForwardVector;
	FVector   CurrentInputOffset     = FVector::ZeroVector;
	FVector2D GamepadInputAxis       = FVector2D::ZeroVector;

	/** 当前有效边界 Volume（弱引用，Volume 销毁后自动失效） */
	TWeakObjectPtr<AYogCameraVolume> ConstraintVolume;

	// ─── 内部逻辑 ─────────────────────────────────────────────────────────

	void SetCameraStates(EYogCameraStates NewState);
	void DetermineState(const FVector& PlayerPos, bool bIsMoving);
	FVector2D ComputeStateOffset(const FVector& PlayerPos) const;
	float GetCurrentLerpSpeed() const;
	AActor* FindNearestPickup(const FVector& PlayerPos) const;
	AYogGameMode* GetYogGameMode() const;
};
