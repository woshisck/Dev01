// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "ANS_AttackRotate.generated.h"

/**
 * AnimNotifyState：攻击窗口内按移动输入方向旋转角色。
 *
 * 与 ANS_AutoTarget（自动锁敌朝向）区别：
 *   本 State 完全受玩家摇杆控制，不依赖敌人位置，
 *   适合需要玩家手动调整攻击方向的攻击段。
 *
 * 用法：
 *   在蒙太奇攻击帧区间上挂此 NotifyState，
 *   角色在窗口内按摇杆方向以 RotateSpeed 度/秒旋转；
 *   摇杆无输入时停止旋转（保持当前朝向）。
 */
UCLASS(meta = (DisplayName = "ANS Attack Rotate"))
class DEVKIT_API UANS_AttackRotate : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	/**
	 * 旋转速度（度/秒）。
	 * 值越大，方向修正越快；0 表示完全锁死朝向。
	 */
	UPROPERTY(EditAnywhere, Category = "AttackRotate", meta = (ClampMin = "0.0", ClampMax = "1080.0"))
	float RotateSpeed = 360.f;

	virtual void NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		float FrameDeltaTime, const FAnimNotifyEventReference& EventReference) override;

	virtual FString GetNotifyName_Implementation() const override;
};
