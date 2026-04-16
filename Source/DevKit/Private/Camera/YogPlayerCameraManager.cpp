// Fill out your copyright notice in the Description page of Project Settings.

#include "Camera/YogPlayerCameraManager.h"

#include "Volume/YogCameraVolume.h"
#include "Character/EnemyCharacterBase.h"
#include "GameFramework/Character.h"
#include "GameModes/YogGameMode.h"
#include "GameModes/LevelFlowTypes.h"
#include "Map/RewardPickup.h"
#include "Kismet/GameplayStatics.h"

// ─────────────────────────────────────────────────────────────────────────────
// 构造函数
// ─────────────────────────────────────────────────────────────────────────────

AYogPlayerCameraManager::AYogPlayerCameraManager(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

// ─────────────────────────────────────────────────────────────────────────────
// 公开 API
// ─────────────────────────────────────────────────────────────────────────────

void AYogPlayerCameraManager::SetDashMode(bool bDashing)
{
	bIsDashing = bDashing;
	if (bDashing)
	{
		MovingTime            = 0.f;
		CurrentLookAheadAlpha = 0.f;
		SetCameraStates(EYogCameraStates::Dash);
	}
	// 退出冲刺：下一帧 DetermineState 自动选择合适状态
}

void AYogPlayerCameraManager::SetCameraInputAxis(FVector2D Axis)
{
	if (Axis.SizeSquared() > 1.f) Axis.Normalize();
	GamepadInputAxis = Axis;
}

void AYogPlayerCameraManager::NotifyHeavyHit()
{
	if (HeavyHitShakeClass)
	{
		StartCameraShake(HeavyHitShakeClass, HeavyHitShakeScale);
	}
}

void AYogPlayerCameraManager::NotifyCritHit()
{
	if (CritHitShakeClass)
	{
		StartCameraShake(CritHitShakeClass, CritHitShakeScale);
	}
}

void AYogPlayerCameraManager::SetConstraintVolume(AYogCameraVolume* Volume)
{
	ConstraintVolume = Volume;
}

// ─────────────────────────────────────────────────────────────────────────────
// 辅助函数
// ─────────────────────────────────────────────────────────────────────────────

AYogGameMode* AYogPlayerCameraManager::GetYogGameMode() const
{
	return Cast<AYogGameMode>(UGameplayStatics::GetGameMode(this));
}

AActor* AYogPlayerCameraManager::FindNearestPickup(const FVector& PlayerPos) const
{
	TArray<AActor*> Pickups;
	UGameplayStatics::GetAllActorsOfClass(this, ARewardPickup::StaticClass(), Pickups);

	AActor* Nearest   = nullptr;
	float   MinDistSq = FLT_MAX;
	for (AActor* P : Pickups)
	{
		if (!IsValid(P)) continue;
		const float D = FVector::DistSquared(P->GetActorLocation(), PlayerPos);
		if (D < MinDistSq) { MinDistSq = D; Nearest = P; }
	}
	return Nearest;
}

void AYogPlayerCameraManager::SetCameraStates(EYogCameraStates NewState)
{
	if (CameraStatus == NewState) return;
	PrevStatus   = CameraStatus;
	CameraStatus = NewState;
}

float AYogPlayerCameraManager::GetCurrentLerpSpeed() const
{
	switch (CameraStatus)
	{
	case EYogCameraStates::FocusCharacter:
		return FocusLerpSpeed;
	case EYogCameraStates::LookAhead:
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

void AYogPlayerCameraManager::DetermineState(const FVector& PlayerPos, bool bIsMoving)
{
	if (bIsDashing)
	{
		SetCameraStates(EYogCameraStates::Dash);
		return;
	}

	AYogGameMode* GM = GetYogGameMode();

	if (GM && GM->CurrentPhase == ELevelPhase::Combat)
	{
		if (GM->HasAliveEnemies())
		{
			const TArray<AEnemyCharacterBase*> Nearby = GM->GetNearbyEnemies(PlayerPos, CombatSearchRadius);
			SetCameraStates(Nearby.Num() > 0
				? EYogCameraStates::CombatFocus
				: EYogCameraStates::CombatSearch);
			return;
		}
	}

	if (GM && GM->CurrentPhase == ELevelPhase::Arrangement)
	{
		if (IsValid(FindNearestPickup(PlayerPos)))
		{
			SetCameraStates(EYogCameraStates::PickupFocus);
			return;
		}
	}

	SetCameraStates(bIsMoving ? EYogCameraStates::LookAhead : EYogCameraStates::FocusCharacter);
}

// ─────────────────────────────────────────────────────────────────────────────
// 偏移计算
// ─────────────────────────────────────────────────────────────────────────────

FVector2D AYogPlayerCameraManager::ComputeStateOffset(const FVector& PlayerPos) const
{
	switch (CameraStatus)
	{
	case EYogCameraStates::FocusCharacter:
	case EYogCameraStates::Dash:
	case EYogCameraStates::Idle:
		return FVector2D::ZeroVector;

	case EYogCameraStates::LookAhead:
	{
		const FVector2D Dir(LastMovementDir.X, LastMovementDir.Y);
		return Dir * LookAheadDistance * CurrentLookAheadAlpha;
	}

	case EYogCameraStates::CombatFocus:
	{
		AYogGameMode* GM = GetYogGameMode();
		if (!GM) return FVector2D::ZeroVector;
		const FVector Centroid = GM->GetEnemyCentroid(PlayerPos, CombatSearchRadius);
		const FVector Dir      = (Centroid - PlayerPos).GetSafeNormal();
		return FVector2D(Dir.X, Dir.Y) * CombatBiasDistance;
	}

	case EYogCameraStates::CombatSearch:
	{
		AYogGameMode* GM = GetYogGameMode();
		if (!GM) return FVector2D::ZeroVector;
		const FVector Dir = GM->GetNearestEnemyDirection(PlayerPos);
		if (Dir.IsNearlyZero()) return FVector2D::ZeroVector;
		return FVector2D(Dir.X, Dir.Y) * CombatSearchOffset;
	}

	case EYogCameraStates::PickupFocus:
	{
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
// 主更新
// ─────────────────────────────────────────────────────────────────────────────

void AYogPlayerCameraManager::UpdateViewTarget(FTViewTarget& OutVT, float DeltaTime)
{
	// Step 1: Super 调用角色 CalcCamera()，SpringArm + CameraComponent 写入基础 POV
	Super::UpdateViewTarget(OutVT, DeltaTime);

	// Step 2: 获取玩家 Pawn
	APawn* ViewedPawn = PCOwner ? PCOwner->GetPawn() : nullptr;
	if (!IsValid(ViewedPawn)) return;

	const FVector PlayerPos      = ViewedPawn->GetActorLocation();
	FVector       PlayerVelocity = FVector::ZeroVector;
	if (ACharacter* Char = Cast<ACharacter>(ViewedPawn))
	{
		PlayerVelocity = Char->GetVelocity();
	}

	const float SpeedXY   = PlayerVelocity.Size2D();
	const bool  bIsMoving = SpeedXY > MovingSpeedThreshold;

	// Step 3: 更新移动时间与前瞻 Alpha
	if (bIsMoving)
	{
		MovingTime += DeltaTime;

		FVector MoveDir2D = PlayerVelocity;
		MoveDir2D.Z = 0.f;
		if (!MoveDir2D.IsNearlyZero())
		{
			LastMovementDir = MoveDir2D.GetSafeNormal();
		}

		const float TargetAlpha = FMath::Clamp(MovingTime / LookAheadBuildupTime, 0.f, 1.f);
		CurrentLookAheadAlpha   = FMath::FInterpTo(CurrentLookAheadAlpha, TargetAlpha, DeltaTime, LookAheadLerpSpeed);
	}
	else
	{
		MovingTime            = 0.f;
		CurrentLookAheadAlpha = FMath::FInterpTo(CurrentLookAheadAlpha, 0.f, DeltaTime, LookAheadAlphaDecaySpeed);
	}

	// Step 4: 状态决策
	DetermineState(PlayerPos, bIsMoving);

	// Step 5: 状态偏移
	const FVector2D StateOffset = ComputeStateOffset(PlayerPos);

	// Step 6: 输入偏移（手柄优先，否则鼠标）
	const bool bGamepadActive = GamepadInputAxis.SizeSquared() > 0.01f;
	FVector2D  ActiveAxis     = GamepadInputAxis;

	if (!bGamepadActive && bAutoReadMouseOffset)
	{
		if (PCOwner)
		{
			float MouseX = 0.f, MouseY = 0.f;
			int32 ViewX = 1,    ViewY = 1;
			PCOwner->GetMousePosition(MouseX, MouseY);
			PCOwner->GetViewportSize(ViewX, ViewY);

			const float NormX =  (MouseX / FMath::Max(ViewX, 1)) * 2.f - 1.f;
			const float NormY = -((MouseY / FMath::Max(ViewY, 1)) * 2.f - 1.f);
			ActiveAxis = FVector2D(NormX, NormY);
		}
	}

	const FVector TargetInputV(ActiveAxis.X * MaxInputOffset, ActiveAxis.Y * MaxInputOffset, 0.f);
	CurrentInputOffset = FMath::VInterpTo(CurrentInputOffset, TargetInputV, DeltaTime, InputOffsetLerpSpeed);

	// Step 7: 合成候选位置（XY 叠加偏移，Z 不动）
	FVector Candidate = OutVT.POV.Location;
	Candidate.X += StateOffset.X + CurrentInputOffset.X;
	Candidate.Y += StateOffset.Y + CurrentInputOffset.Y;

	// Step 8: Volume 边界约束
	// 将候选位置投影到玩家 Z 高度再判断（Volume 通常在地面高度建模）
	if (ConstraintVolume.IsValid())
	{
		const FVector CheckPt(Candidate.X, Candidate.Y, PlayerPos.Z);
		if (!ConstraintVolume->EncompassesPoint(CheckPt))
		{
			// 超出边界：XY 退回到玩家正上方，Z 保持
			Candidate.X = PlayerPos.X;
			Candidate.Y = PlayerPos.Y;
		}
	}

	// Step 9 & 10: 应用位置
	if (CameraStatus == EYogCameraStates::Dash)
	{
		// 冲刺：无插值，直接同步
		OutVT.POV.Location = Candidate;
	}
	else
	{
		const float Speed = GetCurrentLerpSpeed();
		OutVT.POV.Location = FMath::VInterpTo(OutVT.POV.Location, Candidate, DeltaTime, Speed);
	}

#if !UE_BUILD_SHIPPING
	static const UEnum* StateEnum = StaticEnum<EYogCameraStates>();
	if (StateEnum && GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			43, 0.f, FColor::Cyan,
			FString::Printf(TEXT("[CameraManager] State: %s | LookAheadAlpha: %.2f"),
				*StateEnum->GetNameStringByValue(static_cast<int64>(CameraStatus)),
				CurrentLookAheadAlpha));
	}
#endif
}

// ─────────────────────────────────────────────────────────────────────────────
// DisplayDebug
// ─────────────────────────────────────────────────────────────────────────────

void AYogPlayerCameraManager::DisplayDebug(UCanvas* Canvas, const FDebugDisplayInfo& DebugDisplay, float& YL, float& YPos)
{
	// Super::DisplayDebug(Canvas, DebugDisplay, YL, YPos);
}
