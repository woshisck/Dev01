// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "ANS_AutoTarget.generated.h"

/**
 * AnimNotifyState：在攻击窗口内自动索敌并旋转面向最近敌人
 *
 * 用法：
 *   在蒙太奇攻击帧区间上挂此 NotifyState
 *   Begin → 球形搜索+锥角过滤 → 立即旋转朝向最近敌人
 *   Tick  → 若开启 bContinuousTracking，持续追踪目标朝向
 *   End   → 清除缓存目标
 */
UCLASS(meta = (DisplayName = "Auto Target Enemy"))
class DEVKIT_API UANS_AutoTarget : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	/** 搜索半径（cm） */
	UPROPERTY(EditAnywhere, Category = "AutoTarget", meta = (ClampMin = "50.0"))
	float SearchRadius = 350.f;

	/**
	 * 前向锥角（半角，度）
	 * 75 = 150° 宽锥，基本覆盖正面；45 = 90° 较窄；180 = 全向
	 */
	UPROPERTY(EditAnywhere, Category = "AutoTarget", meta = (ClampMin = "10.0", ClampMax = "180.0"))
	float SearchHalfAngleDeg = 75.f;

	/**
	 * 是否在窗口期间持续追踪目标朝向
	 * false（默认）：只在 Begin 时旋转一次，适合快速攻击
	 * true：逐帧追踪，适合蓄力/持续攻击
	 */
	UPROPERTY(EditAnywhere, Category = "AutoTarget")
	bool bContinuousTracking = false;

	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		float TotalDuration, const FAnimNotifyEventReference& EventReference) override;

	virtual void NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		float FrameDeltaTime, const FAnimNotifyEventReference& EventReference) override;

	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;

	virtual FString GetNotifyName_Implementation() const override;

private:
	/** 在搜索范围+锥角内找最优目标（最近的存活敌人） */
	AActor* FindBestTarget(ACharacter* Character) const;

	/** 立即旋转角色面向目标（只修改 Yaw） */
	void SnapToTarget(ACharacter* Character, AActor* Target) const;

	/** 连续追踪时缓存的目标 */
	TWeakObjectPtr<AActor> CachedTarget;
};
