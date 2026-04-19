// Fill out your copyright notice in the Description page of Project Settings.

#include "Animation/ANS_AutoTarget.h"
#include "Character/EnemyCharacterBase.h"
#include "Character/PlayerCharacterBase.h"
#include "Character/YogCharacterBase.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"

void UANS_AutoTarget::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	ACharacter* Character = MeshComp ? Cast<ACharacter>(MeshComp->GetOwner()) : nullptr;
	if (!Character)
		return;

	AActor* Target = FindBestTarget(Character);
	if (Target)
	{
		CachedTarget = Target;
		SnapToTarget(Character, Target);
	}
}

void UANS_AutoTarget::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);

	if (!bContinuousTracking)
		return;

	ACharacter* Character = MeshComp ? Cast<ACharacter>(MeshComp->GetOwner()) : nullptr;
	if (!Character)
		return;

	// 目标失效或已死亡时重新搜索
	AYogCharacterBase* CachedChar = Cast<AYogCharacterBase>(CachedTarget.Get());
	if (!CachedTarget.IsValid() || (CachedChar && !CachedChar->IsAlive()))
		CachedTarget = FindBestTarget(Character);

	if (CachedTarget.IsValid())
		SnapToTarget(Character, CachedTarget.Get());
}

void UANS_AutoTarget::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);
	CachedTarget.Reset();
}

FString UANS_AutoTarget::GetNotifyName_Implementation() const
{
	return FString::Printf(TEXT("AutoTarget R=%.0f A=%.0f"), SearchRadius, SearchHalfAngleDeg);
}

// ─── 私有实现 ────────────────────────────────────────────────────────────────

AActor* UANS_AutoTarget::FindBestTarget(ACharacter* Character) const
{
	if (!Character || !Character->GetWorld())
		return nullptr;

	const FVector Origin = Character->GetActorLocation();

	// 优先用玩家摇杆/移动输入的意图方向；无输入时退回角色朝向
	// 手柄模式下：攻击时玩家按的方向即为索敌锥角基准
	FVector SearchDir = Character->GetActorForwardVector();
	if (const APlayerCharacterBase* PlayerChar = Cast<APlayerCharacterBase>(Character))
	{
		if (!PlayerChar->LastInputDirection.IsNearlyZero())
			SearchDir = PlayerChar->LastInputDirection;
	}

	const float CosHalfAngle = FMath::Cos(FMath::DegreesToRadians(SearchHalfAngleDeg));

	// 球形范围内的所有 Pawn
	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(Character->GetWorld(), AEnemyCharacterBase::StaticClass(), FoundActors);

	AActor* BestTarget = nullptr;
	float BestDistSq = FLT_MAX;

	for (AActor* Actor : FoundActors)
	{
		if (!IsValid(Actor) || Actor == Character)
			continue;

		// 跳过已死亡敌人
		if (AYogCharacterBase* Char = Cast<AYogCharacterBase>(Actor))
		{
			if (!Char->IsAlive())
				continue;
		}

		// 距离过滤
		const FVector ToTarget = Actor->GetActorLocation() - Origin;
		const float DistSq = ToTarget.SizeSquared();
		if (DistSq > SearchRadius * SearchRadius)
			continue;

		// 锥角过滤（以摇杆意图方向为基准，而非角色朝向）
		if (SearchHalfAngleDeg < 180.f)
		{
			const float Dot = FVector::DotProduct(SearchDir, ToTarget.GetSafeNormal());
			if (Dot < CosHalfAngle)
				continue;
		}

		// 取最近
		if (DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			BestTarget = Actor;
		}
	}

	return BestTarget;
}

void UANS_AutoTarget::SnapToTarget(ACharacter* Character, AActor* Target) const
{
	if (!Character || !Target)
		return;

	// 只改 Yaw，保持 Pitch/Roll 为零（俯视角游戏）
	const FVector ToTarget = Target->GetActorLocation() - Character->GetActorLocation();
	const FRotator NewRotation(0.f, ToTarget.Rotation().Yaw, 0.f);
	Character->SetActorRotation(NewRotation);
}
