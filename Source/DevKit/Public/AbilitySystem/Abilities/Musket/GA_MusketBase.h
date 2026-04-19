// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/YogGameplayAbility.h"
#include "Projectile/MusketBullet.h"
#include "Item/Weapon/AimArcActor.h"
#include "GA_MusketBase.generated.h"

class AYogCharacterBase;

/**
 * 火绳枪 GA 基类。
 *
 * 子类 (Blueprint) Class Defaults 需填写：
 *   - BulletClass           → BP_MusketBullet
 *   - BulletDamageEffect    → GE_MusketBullet_Damage（含 SetByCaller Attribute.ActDamage）
 *   - MuzzleSocketName      → 武器/角色 Mesh 上枪口 Socket 名（默认 "Muzzle"）
 *
 * 移动阻断：子类在需要阻断移动时调用 LockMovement() / UnlockMovement()。
 * 弹药操作：调用 ConsumeOneAmmo() / AddOneAmmo() / ClearAllAmmo() / SetAmmoToMax()。
 * 子弹生成：调用 SpawnBullet(YawOffsetDeg, Damage)。
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class DEVKIT_API UGA_MusketBase : public UYogGameplayAbility
{
    GENERATED_BODY()

public:
    UGA_MusketBase();

    // ── 所有子类共用配置（Blueprint Class Defaults 填写）────────────────────────

    /** 子弹 Actor 类 */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Musket|Config")
    TSubclassOf<AMusketBullet> BulletClass;

    /** 伤害 GE（须含 SetByCaller Attribute.ActDamage Modifier） */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Musket|Config")
    TSubclassOf<UGameplayEffect> BulletDamageEffectClass;

    /**
     * 枪口 Socket 名。依次在角色 Mesh、装备武器 Mesh 上查找，
     * 全部找不到时使用角色位置 + 前方 80cm + Z 60cm 的近似值。
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Musket|Config")
    FName MuzzleSocketName = FName("Muzzle");

protected:
    // ── 弹药工具 ────────────────────────────────────────────────────────────────

    float GetCurrentAmmo() const;
    float GetMaxAmmo() const;
    bool  HasAmmo() const;

    /** 直接扣减一发（经 PreAttributeChange 钳制到 [0, MaxAmmo]） */
    void ConsumeOneAmmo();

    /** 直接增加一发 */
    void AddOneAmmo();

    /** 清空所有弹药（冲刺换弹第一步） */
    void ClearAllAmmo();

    /** 将 CurrentAmmo 设为 MaxAmmo */
    void SetAmmoToMax();

    // ── 移动控制 ────────────────────────────────────────────────────────────────

    /** 禁止角色移动（瞄准/蓄力期间调用） */
    void LockMovement();

    /** 恢复角色移动（EndAbility 或发射后调用） */
    void UnlockMovement();

    // ── 子弹生成 ────────────────────────────────────────────────────────────────

    /**
     * 生成一颗子弹并 InitBullet。
     * @param YawOffsetDeg  相对角色 Yaw 的偏移角度（负=左，正=右）
     * @param Damage        传给 InitBullet 的伤害量（调用方已乘倍率）
     */
    AMusketBullet* SpawnBullet(float YawOffsetDeg, float Damage);

    // ── GameplayCue 工具 ─────────────────────────────────────────────────────────

    void ExecuteFireCue();
    void ExecuteReloadCue();
    void ExecuteChargeFullCue();

    // ── 伤害计算辅助 ─────────────────────────────────────────────────────────────

    /** 返回 BaseAttributeSet.Attack 属性值（基础攻击力） */
    float GetBaseAttack() const;

    // ── 内部 ─────────────────────────────────────────────────────────────────────

    /** ActivateAbility 时调用；缓存角色并返回是否有效 */
    bool InitCharacterCache(const FGameplayAbilityActorInfo* ActorInfo);

    /** 占位：UE5.4 GA 无原生 Tick，子类通过 TickAbility 自定义调度（暂未接入引擎 Tick） */
    void SetShouldTick(bool /*bEnabled*/) {}

    UPROPERTY()
    TObjectPtr<AYogCharacterBase> CachedCharacter;

private:
    FVector GetMuzzleLocation() const;

    bool bMovementLocked = false;
};
