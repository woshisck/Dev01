// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "Data/RuneDataAsset.h"
#include "MusketBullet.generated.h"

class USphereComponent;
class UProjectileMovementComponent;
class UGameplayEffect;
class ACharacter;

/**
 * 火绳枪子弹 — 单目标命中即销毁，不穿透。
 *
 * 工作流：
 *   1. GA_Musket_LightAttack / GA_Musket_HeavyAttack 调用 SpawnActor + InitBullet
 *   2. 子弹沿发射方向飞行，SphereComponent 与 Pawn 发生 Overlap
 *   3. 命中第一个有效目标 → ApplyDamageTo → BP_OnHitEnemy → Destroy
 *   4. 生存时间到期未命中 → BP_OnMiss → Destroy
 *
 * 暴击处理：
 *   调用方（GA）在计算好 InDamage 时乘以 Crit 系数后传入，无需此类内部处理。
 *
 * 表现层由 Blueprint 子类实现 BP_OnHitEnemy / BP_OnMiss。
 */
UCLASS(BlueprintType, Blueprintable)
class DEVKIT_API AMusketBullet : public AActor
{
    GENERATED_BODY()

public:
    AMusketBullet();

    /**
     * 发射初始化，SpawnActor 后立即调用。
     * @param InSource        攻击者（用于 EffectContext 和阵营判断）
     * @param InDamage        伤害量，经 GA 计算（含暴击系数）后传入
     * @param InDamageEffect  含 SetByCaller Attribute.ActDamage Modifier 的 GE
     */
    UFUNCTION(BlueprintCallable, Category = "Musket")
    void InitBullet(ACharacter* InSource, float InDamage,
                    TSubclassOf<UGameplayEffect> InDamageEffect);

    UFUNCTION(BlueprintCallable, Category = "Musket|Combat Deck")
    void SetCombatDeckContext(ECardRequiredAction InActionType, bool bInComboFinisher, bool bInFromDashSave);

    void SetCombatDeckContextWithGuid(
        ECardRequiredAction InActionType,
        bool bInComboFinisher,
        bool bInFromDashSave,
        const FGuid& InAttackInstanceGuid,
        float InAttackDamage);

    /** 飞行速度（cm/s），蓄力完成子弹可配置更高速度 */
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Projectile")
    float Speed = 2800.f;

    /** 生存时间（秒），到期销毁（触发 BP_OnMiss） */
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Projectile")
    float Lifetime = 0.8f;

protected:
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<USphereComponent> CollisionSphere;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

    /** 命中有效目标时触发（播放命中特效/音效） */
    UFUNCTION(BlueprintImplementableEvent, Category = "VFX")
    void BP_OnHitEnemy(AActor* HitActor, FVector HitLocation);

    /** 生存时间结束未命中目标（播放消散特效） */
    UFUNCTION(BlueprintImplementableEvent, Category = "VFX")
    void BP_OnMiss();

private:
    UPROPERTY()
    TObjectPtr<ACharacter> SourceCharacter;

    UPROPERTY()
    TSubclassOf<UGameplayEffect> DamageEffectClass;

    float DamageMagnitude = 0.f;
    bool  bHasHit         = false;
    bool  bCombatDeckResolved = false;
    bool  bCombatDeckComboFinisher = false;
    bool  bCombatDeckFromDashSave = false;
    ECardRequiredAction CombatDeckActionType = ECardRequiredAction::Any;
    FGuid CombatDeckAttackInstanceGuid;
    float CombatDeckAttackDamage = 0.f;

    FTimerHandle LifetimeTimerHandle;

    UFUNCTION()
    void OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
        bool bFromSweep, const FHitResult& SweepHitResult);

    void ApplyDamageTo(AActor* Target, const FVector& HitLocation);
    void ResolveCombatDeckOnHit();
    void Expire();
};
