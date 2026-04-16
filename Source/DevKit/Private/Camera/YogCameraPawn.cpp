// Fill out your copyright notice in the Description page of Project Settings.

#include "Camera/YogCameraPawn.h"

#include "AIController.h"
#include "Camera/CameraConstraintActor.h"
#include "Character/EnemyCharacterBase.h"
#include "Character/YogPlayerControllerBase.h"
#include "Character/YogCharacterBase.h"
#include "GameFramework/Character.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "GameFramework/PlayerController.h"
#include "GameModes/YogGameMode.h"
#include "GameModes/LevelFlowTypes.h"
#include "Map/RewardPickup.h"
#include "Camera/PlayerCameraManager.h"
#include "Kismet/GameplayStatics.h"

// ─────────────────────────────────────────────────────────────────────────────
// 构造函数
// ─────────────────────────────────────────────────────────────────────────────

AYogCameraPawn::AYogCameraPawn(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;

	FloatingMovementComponent = ObjectInitializer.CreateDefaultSubobject<UFloatingPawnMovement>(
		this, TEXT("PawnFloatingMovementComp"));

	CameraStatus = EYogCameraStates::FocusCharacter;
	PrevStatus   = EYogCameraStates::FocusCharacter;
}

// ─────────────────────────────────────────────────────────────────────────────
// 生命周期
// ─────────────────────────────────────────────────────────────────────────────

void AYogCameraPawn::BeginPlay()
{
	Super::BeginPlay();

	// 尝试在场景中找到 CameraConstraintActor（懒加载 + 缓存）
	TArray<AActor*> Found;
	UGameplayStatics::GetAllActorsOfClass(this, ACameraConstraintActor::StaticClass(), Found);
	if (Found.Num() > 0)
	{
		CachedConstraintActor = Cast<ACameraConstraintActor>(Found[0]);
	}
}

void AYogCameraPawn::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	if (NewController)
	{
		if (APlayerController* PC = Cast<APlayerController>(NewController))
		{
			UE_LOG(LogTemp, Log, TEXT("AYogCameraPawn: Possessed by PlayerController"));
		}
		else if (AAIController* AIC = Cast<AAIController>(NewController))
		{
			CameraController = AIC;
		}
	}
}

void AYogCameraPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	UpdateCameraLoc(DeltaTime);
}

// ─────────────────────────────────────────────────────────────────────────────
// 公开接口实现
// ─────────────────────────────────────────────────────────────────────────────

void AYogCameraPawn::SetCameraStates(EYogCameraStates NewState)
{
	if (CameraStatus == NewState) return;

	PrevStatus   = CameraStatus;
	CameraStatus = NewState;
	OnCameraStatesChanged(PrevStatus, CameraStatus);
}

void AYogCameraPawn::OnCameraStatesChanged(EYogCameraStates PreviousState, EYogCameraStates NextState)
{
	// 蓝图可覆盖此函数实现过渡效果（淡入淡出、FOV 变化等）
}

void AYogCameraPawn::SetDashMode(bool bDashing)
{
	bIsDashing = bDashing;

	if (bDashing)
	{
		// 进入冲刺：重置前瞻，立即切换状态
		MovingTime            = 0.0f;
		CurrentLookAheadAlpha = 0.0f;
		SetCameraStates(EYogCameraStates::Dash);
	}
	// 退出冲刺时让 DetermineState 在下一帧自动选择合适状态
}

void AYogCameraPawn::SetCameraInputAxis(FVector2D Axis)
{
	// 确保归一化（防止摇杆值超出 [-1,1]）
	if (Axis.SizeSquared() > 1.0f)
	{
		Axis.Normalize();
	}
	GamepadInputAxis = Axis;
}

void AYogCameraPawn::NotifyHeavyHit()
{
	if (!HeavyHitShakeClass) return;

	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (PC && PC->PlayerCameraManager)
	{
		PC->PlayerCameraManager->StartCameraShake(HeavyHitShakeClass, HeavyHitShakeScale);
	}
}

void AYogCameraPawn::NotifyCritHit()
{
	if (!CritHitShakeClass) return;

	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (PC && PC->PlayerCameraManager)
	{
		PC->PlayerCameraManager->StartCameraShake(CritHitShakeClass, CritHitShakeScale);
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// 辅助函数
// ─────────────────────────────────────────────────────────────────────────────

AYogGameMode* AYogCameraPawn::GetGameMode() const
{
	return Cast<AYogGameMode>(UGameplayStatics::GetGameMode(this));
}

ACameraConstraintActor* AYogCameraPawn::GetConstraintActor()
{
	// 若缓存失效（关卡切换等情况），重新查找
	if (!CachedConstraintActor.IsValid())
	{
		TArray<AActor*> Found;
		UGameplayStatics::GetAllActorsOfClass(this, ACameraConstraintActor::StaticClass(), Found);
		if (Found.Num() > 0)
		{
			CachedConstraintActor = Cast<ACameraConstraintActor>(Found[0]);
		}
	}
	return CachedConstraintActor.Get();
}

AActor* AYogCameraPawn::FindNearestPickup(const FVector& PlayerPos) const
{
	TArray<AActor*> Pickups;
	UGameplayStatics::GetAllActorsOfClass(this, ARewardPickup::StaticClass(), Pickups);

	AActor* Nearest    = nullptr;
	float   MinDistSq  = FLT_MAX;

	for (AActor* P : Pickups)
	{
		if (!IsValid(P)) continue;
		const float DistSq = FVector::DistSquared(P->GetActorLocation(), PlayerPos);
		if (DistSq < MinDistSq)
		{
			MinDistSq = DistSq;
			Nearest   = P;
		}
	}
	return Nearest;
}

float AYogCameraPawn::GetCurrentLerpSpeed() const
{
	switch (CameraStatus)
	{
	case EYogCameraStates::FocusCharacter:
		return FocusLerpSpeed;

	case EYogCameraStates::LookAhead:
		// 在 InitialFollowLerpSpeed（初期落后感）和 LookAheadLerpSpeed（前瞻阶段）之间插值
		return FMath::Lerp(InitialFollowLerpSpeed, LookAheadLerpSpeed, CurrentLookAheadAlpha);

	case EYogCameraStates::CombatFocus:
	case EYogCameraStates::CombatSearch:
		return CombatLerpSpeed;

	case EYogCameraStates::PickupFocus:
		return PickupLerpSpeed;

	default:
		return LookAheadLerpSpeed;
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// 状态决策
// ─────────────────────────────────────────────────────────────────────────────

void AYogCameraPawn::DetermineState(const FVector& PlayerPos, bool bIsMoving)
{
	// Priority 1 — 冲刺（由外部 SetDashMode 驱动，退出冲刺后继续向下判断）
	if (bIsDashing)
	{
		SetCameraStates(EYogCameraStates::Dash);
		return;
	}

	AYogGameMode* GM = GetGameMode();

	// Priority 2 & 3 — 战斗阶段
	if (GM && GM->CurrentPhase == ELevelPhase::Combat)
	{
		if (GM->HasAliveEnemies())
		{
			// 判断搜索半径内是否有敌人
			const TArray<AEnemyCharacterBase*> NearbyEnemies = GM->GetNearbyEnemies(PlayerPos, CombatSearchRadius);
			if (NearbyEnemies.Num() > 0)
			{
				// Priority 2：有敌人在搜索半径内 → 质心偏移
				SetCameraStates(EYogCameraStates::CombatFocus);
			}
			else
			{
				// Priority 3：有存活敌人但不在搜索范围 → 向最近敌人偏移
				SetCameraStates(EYogCameraStates::CombatSearch);
			}
			return;
		}
	}

	// Priority 4 — 整理阶段有拾取物
	if (GM && GM->CurrentPhase == ELevelPhase::Arrangement)
	{
		if (IsValid(FindNearestPickup(PlayerPos)))
		{
			SetCameraStates(EYogCameraStates::PickupFocus);
			return;
		}
	}

	// Priority 5 — 玩家正在移动
	if (bIsMoving)
	{
		SetCameraStates(EYogCameraStates::LookAhead);
		return;
	}

	// Priority 6 — 静止聚焦
	SetCameraStates(EYogCameraStates::FocusCharacter);
}

// ─────────────────────────────────────────────────────────────────────────────
// 偏移计算
// ─────────────────────────────────────────────────────────────────────────────

FVector2D AYogCameraPawn::ComputeStateOffset(const FVector& PlayerPos) const
{
	switch (CameraStatus)
	{
	case EYogCameraStates::FocusCharacter:
	case EYogCameraStates::Dash:
	case EYogCameraStates::Idle:
		return FVector2D::ZeroVector;

	case EYogCameraStates::LookAhead:
	{
		// 向上次移动方向的前方偏移，Alpha 从 0 逐步增大到 1
		const FVector2D MoveDir2D(LastMovementDir.X, LastMovementDir.Y);
		return MoveDir2D * LookAheadDistance * CurrentLookAheadAlpha;
	}

	case EYogCameraStates::CombatFocus:
	{
		// 向玩家搜索半径内所有敌人的质心方向偏移
		AYogGameMode* GM = GetGameMode();
		if (!GM) return FVector2D::ZeroVector;

		const FVector Centroid = GM->GetEnemyCentroid(PlayerPos, CombatSearchRadius);
		const FVector Dir      = (Centroid - PlayerPos).GetSafeNormal();
		return FVector2D(Dir.X, Dir.Y) * CombatBiasDistance;
	}

	case EYogCameraStates::CombatSearch:
	{
		// 向最近存活敌人方向偏移
		AYogGameMode* GM = GetGameMode();
		if (!GM) return FVector2D::ZeroVector;

		const FVector Dir = GM->GetNearestEnemyDirection(PlayerPos);
		if (Dir.IsNearlyZero()) return FVector2D::ZeroVector;
		return FVector2D(Dir.X, Dir.Y) * CombatSearchOffset;
	}

	case EYogCameraStates::PickupFocus:
	{
		// 向最近拾取物方向偏移
		AActor* Pickup = FindNearestPickup(PlayerPos);
		if (!IsValid(Pickup)) return FVector2D::ZeroVector;

		const FVector Dir = (Pickup->GetActorLocation() - PlayerPos).GetSafeNormal();
		return FVector2D(Dir.X, Dir.Y) * PickupBiasDistance;
	}

	default:
		return FVector2D::ZeroVector;
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// 主更新函数
// ─────────────────────────────────────────────────────────────────────────────

void AYogCameraPawn::UpdateCameraLoc(float DeltaTime)
{
	AActor* OwnerActor = GetOwner();
	if (!IsValid(OwnerActor)) return;

	// ── 1. 获取玩家位置和速度 ────────────────────────────────────────────

	const FVector PlayerPos = OwnerActor->GetActorLocation();
	FVector PlayerVelocity  = FVector::ZeroVector;

	if (ACharacter* PlayerChar = Cast<ACharacter>(OwnerActor))
	{
		PlayerVelocity = PlayerChar->GetVelocity();
	}

	// 只判断 XY 平面的速度
	const float SpeedXY = PlayerVelocity.Size2D();
	const bool  bIsMoving = SpeedXY > MovingSpeedThreshold;

	// ── 2. 更新移动时间与前瞻 Alpha ───────────────────────────────────────

	if (bIsMoving)
	{
		MovingTime += DeltaTime;

		// 记录最近的移动方向（XY 平面单位向量）
		FVector MoveDir2D = PlayerVelocity;
		MoveDir2D.Z = 0.0f;
		if (!MoveDir2D.IsNearlyZero())
		{
			LastMovementDir = MoveDir2D.GetSafeNormal();
		}

		// Alpha 随移动时间线性增长，到达 1.0 后保持
		const float TargetAlpha = FMath::Clamp(MovingTime / LookAheadBuildupTime, 0.0f, 1.0f);
		CurrentLookAheadAlpha   = FMath::FInterpTo(
			CurrentLookAheadAlpha, TargetAlpha, DeltaTime, LookAheadLerpSpeed);
	}
	else
	{
		// 玩家停止移动：重置计时器，Alpha 快速衰减
		MovingTime            = 0.0f;
		CurrentLookAheadAlpha = FMath::FInterpTo(
			CurrentLookAheadAlpha, 0.0f, DeltaTime, LookAheadAlphaDecaySpeed);
	}

	// ── 3. 状态决策 ───────────────────────────────────────────────────────

	DetermineState(PlayerPos, bIsMoving);

	// ── 4. 计算状态偏移（XY 平面） ────────────────────────────────────────

	const FVector2D StateOffset = ComputeStateOffset(PlayerPos);

	// ── 5. 更新输入偏移（手柄摇杆 / 鼠标，平滑追踪） ─────────────────────

	// 手柄右摇杆未激活时，自动从鼠标位置读取偏移（屏幕中心 = 0，边缘 = ±1）
	// GamepadInputAxis 只由 SetCameraInputAxis() 写入，不会被鼠标值覆盖
	const bool bGamepadActive = GamepadInputAxis.SizeSquared() > 0.01f;
	FVector2D ActiveAxis = GamepadInputAxis;  // 默认使用手柄轴值

	if (!bGamepadActive && bAutoReadMouseOffset)
	{
		if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
		{
			float MouseX = 0.f, MouseY = 0.f;
			int32 ViewX  = 1,   ViewY  = 1;
			PC->GetMousePosition(MouseX, MouseY);
			PC->GetViewportSize(ViewX, ViewY);

			// 归一化到 [-1, 1]，屏幕中心为零点，Y 轴翻转（屏幕 Y 向下）
			const float NormX =  (MouseX / FMath::Max(ViewX, 1)) * 2.0f - 1.0f;
			const float NormY = -((MouseY / FMath::Max(ViewY, 1)) * 2.0f - 1.0f);
			ActiveAxis = FVector2D(NormX, NormY);
		}
	}

	const FVector TargetInputOffsetV(
		ActiveAxis.X * MaxInputOffset,
		ActiveAxis.Y * MaxInputOffset,
		0.0f);

	CurrentInputOffset = FMath::VInterpTo(
		CurrentInputOffset, TargetInputOffsetV, DeltaTime, InputOffsetLerpSpeed);

	// ── 6. 合成目标 XY 位置 ───────────────────────────────────────────────

	FVector2D TargetXY(
		PlayerPos.X + StateOffset.X + CurrentInputOffset.X,
		PlayerPos.Y + StateOffset.Y + CurrentInputOffset.Y);

	// ── 7. 多边形边界约束（XY） ────────────────────────────────────────────

	if (ACameraConstraintActor* Constraint = GetConstraintActor())
	{
		TargetXY = Constraint->ConstrainPosition(TargetXY);
	}

	// ── 8. 构建目标位置（Z 跟随玩家） ─────────────────────────────────────

	// Z 值直接跟随玩家，确保地形起伏时相机保持正确高度
	// 实际摄像机高度由蓝图中 SpringArm 或 CameraComponent 的相对偏移控制
	const FVector TargetPos(TargetXY.X, TargetXY.Y, PlayerPos.Z);

	// ── 9. 应用位置 ───────────────────────────────────────────────────────

	if (CameraStatus == EYogCameraStates::Dash)
	{
		// 冲刺：直接设置位置，不经过插值，保证与玩家完全同步
		SetActorLocation(TargetPos);
	}
	else
	{
		const float LerpSpeed = GetCurrentLerpSpeed();
		const FVector NewPos  = FMath::VInterpTo(
			GetActorLocation(), TargetPos, DeltaTime, LerpSpeed);
		SetActorLocation(NewPos);
	}

#if !UE_BUILD_SHIPPING
	// ── 调试信息 ──────────────────────────────────────────────────────────
	// 在屏幕左上角显示当前相机状态（开发版本）
	static const UEnum* StateEnum = StaticEnum<EYogCameraStates>();
	if (StateEnum && GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			42, 0.0f, FColor::Cyan,
			FString::Printf(TEXT("[Camera] State: %s | LookAheadAlpha: %.2f"),
				*StateEnum->GetNameStringByValue(static_cast<int64>(CameraStatus)),
				CurrentLookAheadAlpha));
	}
#endif
}
