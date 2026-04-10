// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/YogTargetType.h"
#include "Data/AbilityData.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "YogTargetType_Melee.generated.h"

class AEnemyCharacterBase;
class APlayerCharacterBase;

/**
 * 近战命中检测基类，封装 Annulus / Triangle-fan 两种命中框逻辑。
 * 子类只需重写 GetTargets_Implementation，指定搜索目标类型（敌人/玩家）。
 */
UCLASS(Abstract, NotBlueprintable)
class DEVKIT_API UYogTargetType_MeleeBase : public UYogTargetType
{
	GENERATED_BODY()

protected:
	/**
	 * 读取 FActionData。
	 * 优先从 EventData.OptionalObject（GA 自身）读取，保证拿到正确的技能实例；
	 * 回退到 ASC->GetCurrentAbilityInstance()（兼容不传 OptionalObject 的旧调用路径）。
	 */
	FActionData GetActionData(AYogCharacterBase* TargetingCharacter, const FGameplayEventData& EventData) const;

	/** 根据 ActionData 的 hitboxTypes 判断目标是否在命中框内 */
	bool IsTargetHit(const FVector& CharLoc, float CharYaw, const FActionData& ActionData, const FVector& TargetLoc) const;

	/** 环形扇区命中检测 */
	bool IsInAnnulus(const FVector& CharLoc, float CharYaw, const FHitboxAnnulus& Annulus, float OuterRadius, const FVector& TargetLoc) const;

	/** 三角形扇面命中检测（对应 Blueprint B_TT 的 Triangle 逻辑） */
	bool IsInTriangleFan(const FVector& CharLoc, float CharYaw, float Range, const TArray<FHitboxTriangle>& Triangles, const FVector& TargetLoc) const;

	/** 2D 三角形内部点检测（叉积符号法） */
	bool IsPointInTriangle2D(const FVector2D& A, const FVector2D& B, const FVector2D& C, const FVector2D& P) const;
};


/**
 * 敌人攻击玩家（C++ 替代 Blueprint B_TT_Enemy）
 * 读取当前激活技能的 ActionData，对场景内 APlayerCharacterBase 做命中框筛选。
 */
UCLASS(NotBlueprintable)
class DEVKIT_API UYogTargetType_Enemy : public UYogTargetType_MeleeBase
{
	GENERATED_BODY()

public:
	virtual void GetTargets_Implementation(
		AYogCharacterBase* TargetingCharacter, AActor* TargetingActor, FGameplayEventData EventData,
		TArray<FHitResult>& OutHitResults, TArray<AActor*>& OutActors) const override;
};


/**
 * 玩家攻击敌人（C++ 替代 Blueprint B_TT_Player）
 * 读取当前激活技能的 ActionData，对场景内 AEnemyCharacterBase 做命中框筛选。
 */
UCLASS(NotBlueprintable)
class DEVKIT_API UYogTargetType_Player : public UYogTargetType_MeleeBase
{
	GENERATED_BODY()

public:
	virtual void GetTargets_Implementation(
		AYogCharacterBase* TargetingCharacter, AActor* TargetingActor, FGameplayEventData EventData,
		TArray<FHitResult>& OutHitResults, TArray<AActor*>& OutActors) const override;
};
