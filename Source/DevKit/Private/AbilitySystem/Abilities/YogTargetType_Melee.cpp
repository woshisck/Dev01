// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/Abilities/YogTargetType_Melee.h"
#include "AbilitySystem/Abilities/YogGameplayAbility.h"
#include "AbilitySystem/Abilities/GA_MeleeAttack.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "Animation/AN_MeleeDamage.h"
#include "Character/YogCharacterBase.h"
#include "Character/EnemyCharacterBase.h"
#include "Character/PlayerCharacterBase.h"
#include "Data/MontageAttackDataAsset.h"
#include "Kismet/KismetSystemLibrary.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Controller.h"

// ── 命中框 Debug 绘制（仅 Development 构建） ─────────────────────────────
#if ENABLE_DRAW_DEBUG

static void DrawHitboxDebug(UWorld* World, const FVector& Loc, float Yaw,
                             const FActionData& Data, FColor Color, float Duration = 0.5f)
{
	if (!World) return;

	const float OuterR = Data.ActRange > 0.f ? Data.ActRange : 400.f;

	if (Data.hitboxTypes.IsEmpty())
	{
		// 无命中框配置 → 画完整圆
		constexpr int32 Seg = 32;
		FVector Prev = Loc + FVector(OuterR, 0, 0);
		for (int32 i = 1; i <= Seg; ++i)
		{
			float Rad = FMath::DegreesToRadians(360.f * i / Seg);
			FVector Cur = Loc + FVector(FMath::Cos(Rad) * OuterR, FMath::Sin(Rad) * OuterR, 0);
			DrawDebugLine(World, Prev, Cur, Color, false, Duration);
			Prev = Cur;
		}
		return;
	}

	for (const FYogHitboxType& HB : Data.hitboxTypes)
	{
		if (HB.hitboxType == EHitBoxType::Annulus)
		{
			// 弧形扇区
			const FHitboxAnnulus& Ann = HB.AnnulusHitbox;
			const float InnerR  = Ann.inner_radius;
			const float HalfDeg = Ann.degree * 0.5f;
			const float StartDeg = Yaw - HalfDeg;
			const float EndDeg   = Yaw + HalfDeg;

			// 计算调试线框的圆心（与 IsInAnnulus 保持一致）
			const float EffectiveOffset = Ann.bAutoOffset ? -InnerR : Ann.OffsetCore;
			const float YawRad = FMath::DegreesToRadians(Yaw);
			const FVector CenterLoc = Loc + FVector(FMath::Cos(YawRad), FMath::Sin(YawRad), 0.f) * EffectiveOffset;

			// bAutoOffset 时圆心后移了 InnerR，外径补偿使前向最终触达 = ActRange（配表直觉一致）
			const float EffectiveOuterR = (Ann.bAutoOffset && InnerR > 0.f) ? OuterR + InnerR : OuterR;

			constexpr int32 Seg = 24;
			FVector PrevOuter = FVector::ZeroVector;
			FVector PrevInner = FVector::ZeroVector;

			for (int32 i = 0; i <= Seg; ++i)
			{
				float Deg = FMath::Lerp(StartDeg, EndDeg, (float)i / Seg);
				float Rad = FMath::DegreesToRadians(Deg);
				FVector Outer = CenterLoc + FVector(FMath::Cos(Rad) * EffectiveOuterR, FMath::Sin(Rad) * EffectiveOuterR, 0);
				FVector Inner = CenterLoc + FVector(FMath::Cos(Rad) * InnerR, FMath::Sin(Rad) * InnerR, 0);
				if (i > 0)
				{
					DrawDebugLine(World, PrevOuter, Outer, Color, false, Duration);
					if (InnerR > 0) DrawDebugLine(World, PrevInner, Inner, Color, false, Duration);
				}
				PrevOuter = Outer; PrevInner = Inner;
			}

			// 两条侧边（内圆 → 外弧；若无内圆则从圆心出发）
			auto RadEdge = [&](float Deg, float R) {
				float Rad = FMath::DegreesToRadians(Deg);
				return CenterLoc + FVector(FMath::Cos(Rad) * R, FMath::Sin(Rad) * R, 0);
			};
			DrawDebugLine(World, InnerR > 0 ? RadEdge(StartDeg, InnerR) : CenterLoc, RadEdge(StartDeg, EffectiveOuterR), Color, false, Duration);
			DrawDebugLine(World, InnerR > 0 ? RadEdge(EndDeg,   InnerR) : CenterLoc, RadEdge(EndDeg,   EffectiveOuterR), Color, false, Duration);
		}
		else if (HB.hitboxType == EHitBoxType::Triangle)
		{
			// 三角扇面
			const TArray<FHitboxTriangle>& Tri = HB.HitboxTriangles;
			for (int32 i = 0; i < Tri.Num(); ++i)
			{
				float Rad = FMath::DegreesToRadians(Yaw + Tri[i].Degree);
				FVector Pt = Loc + FVector(FMath::Cos(Rad) * OuterR, FMath::Sin(Rad) * OuterR, 0);
				DrawDebugLine(World, Loc, Pt, Color, false, Duration);
				if (i > 0)
				{
					float PrevRad = FMath::DegreesToRadians(Yaw + Tri[i - 1].Degree);
					FVector PrevPt = Loc + FVector(FMath::Cos(PrevRad) * OuterR, FMath::Sin(PrevRad) * OuterR, 0);
					DrawDebugLine(World, PrevPt, Pt, Color, false, Duration);
				}
			}
		}
	}
}

#endif // ENABLE_DRAW_DEBUG

// ===========================================================
// UYogTargetType_MeleeBase — 共用逻辑
// ===========================================================

FActionData UYogTargetType_MeleeBase::GetActionData(AYogCharacterBase* TargetingCharacter, const FGameplayEventData& EventData) const
{
	if (TargetingCharacter)
	{
		if (UYogAbilitySystemComponent* ASC = TargetingCharacter->GetASC())
		{
			if (const UGA_MeleeAttack* MeleeGA = Cast<UGA_MeleeAttack>(ASC->GetCurrentAbilityInstance()))
			{
				if (MeleeGA->HasConfiguredAttackData())
				{
					return MeleeGA->GetAbilityActionData();
				}
			}
		}
	}
	// 优先从 OptionalObject 拿：AN_MeleeDamage::Notify 已将自身设置为 OptionalObject
	if (const UAN_MeleeDamage* DmgNotify = Cast<UAN_MeleeDamage>(EventData.OptionalObject))
	{
		FActionData Data = DmgNotify->BuildActionData();
		UE_LOG(LogTemp, Warning, TEXT("[TargetType] ActionData(from Notify): ActRange=%.1f, hitboxTypes=%d"),
			Data.ActRange, Data.hitboxTypes.Num());
		return Data;
	}

	if (const UMontageAttackDataAsset* AttackData = Cast<UMontageAttackDataAsset>(EventData.OptionalObject))
	{
		return AttackData->BuildActionData();
	}

	// 回退（旧路径兼容）：从 GA 读取
	if (const UGA_MeleeAttack* MeleeGA = Cast<UGA_MeleeAttack>(EventData.OptionalObject))
	{
		return MeleeGA->GetAbilityActionData();
	}

	// 最终回退：从 ASC 当前激活技能读取
	if (!TargetingCharacter) return FActionData();
	UYogAbilitySystemComponent* ASC = TargetingCharacter->GetASC();
	if (!ASC) return FActionData();

	UYogGameplayAbility* CurrentAbility = ASC->GetCurrentAbilityInstance();
	if (CurrentAbility)
	{
		return CurrentAbility->GetAbilityActionData();
	}

	return FActionData();
}

bool UYogTargetType_MeleeBase::IsTargetHit(const FVector& CharLoc, float CharYaw, const FActionData& ActionData, const FVector& TargetLoc) const
{
	const float OuterRadius = ActionData.ActRange > 0.f ? ActionData.ActRange : 400.f;

	if (ActionData.hitboxTypes.IsEmpty())
	{
		// 无命中框配置：全向球形判断
		return FVector::Dist2D(CharLoc, TargetLoc) <= OuterRadius;
	}

	for (const FYogHitboxType& Hitbox : ActionData.hitboxTypes)
	{
		switch (Hitbox.hitboxType)
		{
		case EHitBoxType::Annulus:
			if (IsInAnnulus(CharLoc, CharYaw, Hitbox.AnnulusHitbox, OuterRadius, TargetLoc))
				return true;
			break;

		case EHitBoxType::Triangle:
			if (IsInTriangleFan(CharLoc, CharYaw, OuterRadius, Hitbox.HitboxTriangles, TargetLoc))
				return true;
			break;

		default:
			// Square / Circle：暂用球形兜底
			if (FVector::Dist2D(CharLoc, TargetLoc) <= OuterRadius)
				return true;
			break;
		}
	}

	return false;
}

bool UYogTargetType_MeleeBase::IsInAnnulus(
	const FVector& CharLoc, float CharYaw,
	const FHitboxAnnulus& Annulus, float OuterRadius,
	const FVector& TargetLoc) const
{
	// 计算圆心位置：自动模式后移 inner_radius，手动模式用 OffsetCore（正=前，负=后）
	const float EffectiveOffset = Annulus.bAutoOffset ? -Annulus.inner_radius : Annulus.OffsetCore;
	const float YawRad = FMath::DegreesToRadians(CharYaw);
	const FVector CharForward(FMath::Cos(YawRad), FMath::Sin(YawRad), 0.f);
	const FVector CenterLoc = CharLoc + CharForward * EffectiveOffset;

	// 距离判断（基于偏移后的圆心）
	// bAutoOffset 时圆心后移了 inner_radius，外径补偿使前向最终触达 = ActRange（配表直觉一致）
	const float Dist2D = FVector::Dist2D(CenterLoc, TargetLoc);
	const float EffectiveOuterRadius = (Annulus.bAutoOffset && Annulus.inner_radius > 0.f)
		? OuterRadius + Annulus.inner_radius
		: OuterRadius;
	if (Dist2D < Annulus.inner_radius || Dist2D > EffectiveOuterRadius)
		return false;

	// 角度判断：圆心 → 目标的方位角，与角色朝向对比
	const float AngleToTarget = FMath::RadiansToDegrees(
		FMath::Atan2(TargetLoc.Y - CenterLoc.Y, TargetLoc.X - CenterLoc.X));

	const float DeltaAngle = FMath::Abs(FMath::FindDeltaAngleDegrees(AngleToTarget, CharYaw));

	return DeltaAngle <= Annulus.degree * 0.5f;
}

bool UYogTargetType_MeleeBase::IsInTriangleFan(
	const FVector& CharLoc, float CharYaw, float Range,
	const TArray<FHitboxTriangle>& Triangles,
	const FVector& TargetLoc) const
{
	if (Triangles.Num() < 2) return false;

	const FVector2D Origin(CharLoc.X, CharLoc.Y);
	const FVector2D TargetXY(TargetLoc.X, TargetLoc.Y);

	// 每相邻两个角度定义一个三角形扇面：Origin + PointB + PointC
	for (int32 i = 1; i < Triangles.Num(); ++i)
	{
		const float Rad0 = FMath::DegreesToRadians(CharYaw + Triangles[i - 1].Degree);
		const float Rad1 = FMath::DegreesToRadians(CharYaw + Triangles[i].Degree);

		const FVector2D PointB(
			CharLoc.X + FMath::Cos(Rad0) * Range,
			CharLoc.Y + FMath::Sin(Rad0) * Range);

		const FVector2D PointC(
			CharLoc.X + FMath::Cos(Rad1) * Range,
			CharLoc.Y + FMath::Sin(Rad1) * Range);

		if (IsPointInTriangle2D(Origin, PointB, PointC, TargetXY))
			return true;
	}

	return false;
}

bool UYogTargetType_MeleeBase::IsPointInTriangle2D(
	const FVector2D& A, const FVector2D& B, const FVector2D& C,
	const FVector2D& P) const
{
	// 叉积符号法：所有符号相同则点在三角形内
	auto CrossSign = [](const FVector2D& P1, const FVector2D& P2, const FVector2D& P3) -> float
	{
		return (P1.X - P3.X) * (P2.Y - P3.Y) - (P2.X - P3.X) * (P1.Y - P3.Y);
	};

	const float d1 = CrossSign(P, A, B);
	const float d2 = CrossSign(P, B, C);
	const float d3 = CrossSign(P, C, A);

	const bool has_neg = (d1 < 0.f) || (d2 < 0.f) || (d3 < 0.f);
	const bool has_pos = (d1 > 0.f) || (d2 > 0.f) || (d3 > 0.f);

	return !(has_neg && has_pos);
}

// ===========================================================
// UYogTargetType_Enemy — 敌人攻击玩家
// ===========================================================

void UYogTargetType_Enemy::GetTargets_Implementation(
	AYogCharacterBase* TargetingCharacter, AActor* TargetingActor,
	FGameplayEventData EventData,
	TArray<FHitResult>& OutHitResults, TArray<AActor*>& OutActors) const
{
	if (!TargetingCharacter) return;

	const FActionData ActionData = GetActionData(TargetingCharacter, EventData);
	const FVector CharLoc = TargetingCharacter->GetActorLocation();
	const float ActorYaw = TargetingCharacter->GetActorRotation().Yaw;
	float ControlYaw = ActorYaw;
	if (const AController* Controller = TargetingCharacter->GetController())
	{
		ControlYaw = Controller->GetControlRotation().Yaw;
	}
	const float CharYaw = ActorYaw;
	const float    SearchRadius = ActionData.ActRange > 0.f ? ActionData.ActRange : 400.f;

	TArray<AActor*> Overlapped;
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));
	TArray<AActor*> Ignore;
	Ignore.Add(TargetingCharacter);

	UKismetSystemLibrary::SphereOverlapActors(
		TargetingCharacter->GetWorld(), CharLoc, SearchRadius,
		ObjectTypes, APlayerCharacterBase::StaticClass(), Ignore, Overlapped);

	int32 AlivePlayerCandidates = 0;
	float ClosestAlivePlayerDist = TNumericLimits<float>::Max();
	FString ClosestAlivePlayerName = TEXT("none");

	for (AActor* Actor : Overlapped)
	{
		APlayerCharacterBase* Player = Cast<APlayerCharacterBase>(Actor);
		if (!Player || !Player->IsAlive()) continue;

		++AlivePlayerCandidates;
		const float Dist2D = FVector::Dist2D(CharLoc, Player->GetActorLocation());
		if (Dist2D < ClosestAlivePlayerDist)
		{
			ClosestAlivePlayerDist = Dist2D;
			ClosestAlivePlayerName = Player->GetName();
		}

		const bool bHit = IsTargetHit(CharLoc, CharYaw, ActionData, Player->GetActorLocation());
		UE_LOG(LogTemp, Warning,
			TEXT("[EnemyAttackTrace] Candidate Enemy=%s Player=%s Dist=%.1f Range=%.1f ActorYaw=%.1f ControlYaw=%.1f Hit=%d HitboxTypes=%d"),
			*GetNameSafe(TargetingCharacter), *GetNameSafe(Player), Dist2D, SearchRadius, ActorYaw, ControlYaw,
			bHit ? 1 : 0, ActionData.hitboxTypes.Num());

		if (bHit)
		{
			OutActors.Add(Player);
		}
	}

	if (OutActors.IsEmpty())
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[EnemyAttackTrace] NoHit Enemy=%s Overlap=%d AliveCandidates=%d Closest=%s ClosestDist=%.1f Range=%.1f ActorYaw=%.1f ControlYaw=%.1f ActDamage=%.1f HitboxTypes=%d"),
			*GetNameSafe(TargetingCharacter), Overlapped.Num(), AlivePlayerCandidates, *ClosestAlivePlayerName,
			ClosestAlivePlayerDist == TNumericLimits<float>::Max() ? -1.f : ClosestAlivePlayerDist,
			SearchRadius, ActorYaw, ControlYaw, ActionData.ActDamage, ActionData.hitboxTypes.Num());
	}

#if ENABLE_DRAW_DEBUG
	{
		const UAN_MeleeDamage* DmgNotify = Cast<UAN_MeleeDamage>(EventData.OptionalObject);
		if (!DmgNotify || DmgNotify->DrawDebug)
		{
			DrawHitboxDebug(TargetingCharacter->GetWorld(), CharLoc, CharYaw, ActionData, FColor::Orange);
		}
	}
#endif
}

// ===========================================================
// UYogTargetType_Player — 玩家攻击敌人
// ===========================================================

void UYogTargetType_Player::GetTargets_Implementation(
	AYogCharacterBase* TargetingCharacter, AActor* TargetingActor,
	FGameplayEventData EventData,
	TArray<FHitResult>& OutHitResults, TArray<AActor*>& OutActors) const
{
	if (!TargetingCharacter) return;

	const FActionData ActionData = GetActionData(TargetingCharacter, EventData);
	const FVector  CharLoc     = TargetingCharacter->GetActorLocation();
	const float    CharYaw     = TargetingCharacter->GetActorRotation().Yaw;
	const float    SearchRadius = ActionData.ActRange > 0.f ? ActionData.ActRange : 400.f;

	TArray<AActor*> Overlapped;
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));
	TArray<AActor*> Ignore;
	Ignore.Add(TargetingCharacter);

	UKismetSystemLibrary::SphereOverlapActors(
		TargetingCharacter->GetWorld(), CharLoc, SearchRadius,
		ObjectTypes, AEnemyCharacterBase::StaticClass(), Ignore, Overlapped);

	for (AActor* Actor : Overlapped)
	{
		AEnemyCharacterBase* Enemy = Cast<AEnemyCharacterBase>(Actor);
		if (!Enemy || !Enemy->IsAlive()) continue;

		if (IsTargetHit(CharLoc, CharYaw, ActionData, Enemy->GetActorLocation()))
		{
			OutActors.Add(Enemy);
		}
	}

#if ENABLE_DRAW_DEBUG
	{
		const UAN_MeleeDamage* DmgNotify = Cast<UAN_MeleeDamage>(EventData.OptionalObject);
		if (!DmgNotify || DmgNotify->DrawDebug)
		{
			DrawHitboxDebug(TargetingCharacter->GetWorld(), CharLoc, CharYaw, ActionData, FColor::Yellow);
		}
	}
#endif
}
