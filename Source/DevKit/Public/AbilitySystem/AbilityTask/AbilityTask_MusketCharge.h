// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "AbilityTask_MusketCharge.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FMusketChargeTickDelegate,
    float, HalfAngleDeg, float, RadiusCm, bool, bFullCharge);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMusketChargeFullDelegate);

/**
 * 蓄力帧更新任务。
 *
 * bTickingTask = true，GAS 每帧调用 TickTask，向 GA 广播当前弧参数。
 * GA 侧绑定 OnChargeTick / OnChargeFull，无需依赖 UGameplayAbility::TickAbility。
 */
UCLASS()
class DEVKIT_API UAbilityTask_MusketCharge : public UAbilityTask
{
    GENERATED_BODY()

public:
    UAbilityTask_MusketCharge(const FObjectInitializer& ObjectInitializer);

    /** 每帧广播：当前弧半角（度）、弧半径（cm）、是否已满蓄 */
    UPROPERTY(BlueprintAssignable)
    FMusketChargeTickDelegate OnChargeTick;

    /** 蓄力第一次达到满蓄时广播一次 */
    UPROPERTY(BlueprintAssignable)
    FMusketChargeFullDelegate OnChargeFull;

    /**
     * 创建蓄力帧更新任务。
     * @param InChargeTime      蓄力满所需秒数
     * @param InStartHalfAngle  起始半角（度）
     * @param InEndHalfAngle    满蓄半角（度）
     * @param InStartRadius     起始半径（cm）
     * @param InEndRadius       满蓄半径（cm）
     */
    UFUNCTION(BlueprintCallable, Category = "Ability|Tasks|Musket",
              meta = (HidePin = "OwningAbility", DefaultToSelf = "OwningAbility",
                      BlueprintInternalUseOnly = "TRUE"))
    static UAbilityTask_MusketCharge* CreateMusketCharge(
        UGameplayAbility* OwningAbility,
        float InChargeTime,
        float InStartHalfAngle,
        float InEndHalfAngle,
        float InStartRadius,
        float InEndRadius);

    virtual void Activate() override;
    virtual void TickTask(float DeltaTime) override;
    virtual void OnDestroy(bool bAbilityEnded) override;

    /** 当前蓄力进度 [0,1] */
    float GetChargeAlpha() const;

    /** 是否已满蓄 */
    bool IsFullCharge() const { return bFullNotified; }

    /** 当前弧半角（度），供 DoFire 读取 */
    float GetCurrentHalfAngle() const { return CurrentHalfAngle; }

private:
    float ChargeTime     = 1.8f;
    float StartHalfAngle = 45.f;
    float EndHalfAngle   = 8.f;
    float StartRadius    = 300.f;
    float EndRadius      = 1200.f;

    float ChargeElapsed   = 0.f;
    bool  bFullNotified   = false;
    float CurrentHalfAngle = 45.f;
};
