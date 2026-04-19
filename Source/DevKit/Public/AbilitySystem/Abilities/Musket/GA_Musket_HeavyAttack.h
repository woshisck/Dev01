// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/Musket/GA_MusketBase.h"
#include "GA_Musket_HeavyAttack.generated.h"

class UAbilityTask_MusketCharge;

/**
 * 重攻击蓄力瞄准 GA。
 *
 * 工作流：
 *   检查弹药 → 阻断移动 → 生成 AimArcActor → 启动 UAbilityTask_MusketCharge（bTickingTask）
 *   → 每帧 OnChargeTick 更新弧 → WaitInputRelease 松手 → DoFire → 播放蒙太奇 → EndAbility
 *
 * 满蓄力（ChargeTime 到达）：伤害 ×FullChargeMultiplier，弧色变为 FullChargeArcColor。
 *
 * Blueprint 子类 Class Defaults 需填写：
 *   - AimArcClass             BP_AimArcActor 的子类（带 M_AimArc 材质）
 *   - FireMontage             开枪蒙太奇
 *   - ChargeTime              蓄力总时长（秒）
 *   - Start/EndHalfAngle      蓄力起止扇形半角
 *   - Start/EndRadius         蓄力起止扇形半径
 *   - BaseDamageMultiplier    蓄力期间松手的伤害倍率
 *   - FullChargeMultiplier    蓄力满时的伤害倍率
 */
UCLASS(BlueprintType, Blueprintable)
class DEVKIT_API UGA_Musket_HeavyAttack : public UGA_MusketBase
{
    GENERATED_BODY()

public:
    UGA_Musket_HeavyAttack();

    /** 瞄准弧指示器 Actor 类（Blueprint 填写 BP_AimArcActor 子类） */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Musket|HeavyAtk")
    TSubclassOf<AYogAimArcActor> AimArcClass;

    /** 开枪蒙太奇 */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Musket|HeavyAtk")
    TObjectPtr<UAnimMontage> FireMontage;

    /** 蓄力完成所需时间（秒） */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Musket|HeavyAtk",
              meta = (ClampMin = "0.3", ClampMax = "5.0"))
    float ChargeTime = 1.8f;

    /** 蓄力开始时扇形半角（度） */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Musket|HeavyAtk",
              meta = (ClampMin = "1.0", ClampMax = "90.0"))
    float StartHalfAngle = 45.f;

    /** 蓄力完成时扇形半角（度，不为 0 保留随机性） */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Musket|HeavyAtk",
              meta = (ClampMin = "0.5", ClampMax = "45.0"))
    float EndHalfAngle = 8.f;

    /** 蓄力开始时扇形半径（cm） */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Musket|HeavyAtk",
              meta = (ClampMin = "50.0"))
    float StartRadius = 300.f;

    /** 蓄力完成时扇形半径（cm） */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Musket|HeavyAtk",
              meta = (ClampMin = "50.0"))
    float EndRadius = 1200.f;

    /** 未满蓄时松手的伤害倍率 */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Musket|HeavyAtk",
              meta = (ClampMin = "0.1"))
    float BaseDamageMultiplier = 1.2f;

    /** 蓄力满时的伤害倍率 */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Musket|HeavyAtk",
              meta = (ClampMin = "0.1"))
    float FullChargeMultiplier = 2.0f;

    /** 普通蓄力弧颜色 */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Musket|HeavyAtk")
    FLinearColor NormalArcColor = FLinearColor(1.f, 0.55f, 0.f, 1.f);

    /** 蓄力满时弧颜色 */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Musket|HeavyAtk")
    FLinearColor FullChargeArcColor = FLinearColor(1.f, 1.f, 0.1f, 1.f);

protected:
    virtual void ActivateAbility(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo,
        const FGameplayEventData* TriggerEventData) override;

    virtual void EndAbility(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo,
        bool bReplicateEndAbility,
        bool bWasCancelled) override;

private:
    float CurrentHalfAngle = 45.f;
    bool  bFullCharge      = false;
    bool  bFired           = false;

    UPROPERTY()
    TObjectPtr<AYogAimArcActor> AimArcActor;

    UPROPERTY()
    TObjectPtr<UAbilityTask_MusketCharge> ChargeTask;

    void DoFire();
    void CleanupArc();

    UFUNCTION() void OnChargeTick(float HalfAngleDeg, float RadiusCm, bool bFull);
    UFUNCTION() void OnChargeFullNotify();
    UFUNCTION() void OnInputReleased(float TimeHeld);

    UFUNCTION() void OnFireMontageComplete(FGameplayTag EventTag, FGameplayEventData EventData);
    UFUNCTION() void OnFireMontageBlendOut(FGameplayTag EventTag, FGameplayEventData EventData);
    UFUNCTION() void OnFireMontageInterrupted(FGameplayTag EventTag, FGameplayEventData EventData);
    UFUNCTION() void OnFireMontageCancelled(FGameplayTag EventTag, FGameplayEventData EventData);
};
