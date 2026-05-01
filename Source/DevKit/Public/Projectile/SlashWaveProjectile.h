#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "SlashWaveProjectile.generated.h"

class UBoxComponent;
class UProjectileMovementComponent;
class UGameplayEffect;
class ACharacter;

/**
 * 刀光投射物 — 水平飞行，穿透多个敌人，Timer 到期后销毁。
 *
 * 工作流：
 *   1. GA_SlashWaveCounter 调用 SpawnActor<ASlashWaveProjectile>，然后 InitProjectile
 *   2. 投射物沿发射方向飞行，CollisionBox 与 Pawn 发生 Overlap
 *   3. 每次命中新目标 → 施加 DamageEffect（SetByCaller Attribute.ActDamage）
 *   4. 已命中目标跳过（HitActors 穿透记录）
 *   5. 生存时间到期 → BP_OnExpired → Destroy
 *
 * 表现层（刀光形状/粒子/音效）由 Blueprint 子类实现 BP_OnHitEnemy / BP_OnExpired。
 */
UCLASS(BlueprintType, Blueprintable)
class DEVKIT_API ASlashWaveProjectile : public AActor
{
	GENERATED_BODY()

public:
	ASlashWaveProjectile();

	/**
	 * 发射初始化，必须在 SpawnActor 后立即调用。
	 * @param InSource        攻击发起者（用于 GE EffectContext 和阵营判断）
	 * @param InDamage        刀光伤害量，通过 SetByCaller Attribute.ActDamage 传递
	 * @param InDamageEffect  施加到目标的 GE 类（须含 SetByCaller Attribute.ActDamage Modifier）
	 */
	UFUNCTION(BlueprintCallable, Category = "SlashWave")
	void InitProjectile(ACharacter* InSource, float InDamage,
	                    TSubclassOf<UGameplayEffect> InDamageEffect);

	UFUNCTION(BlueprintCallable, Category = "SlashWave")
	void InitProjectileAdvanced(
		ACharacter* InSource,
		float InDamage,
		TSubclassOf<UGameplayEffect> InDamageEffect,
		float InSpeed,
		float InMaxDistance,
		int32 InMaxHitCount,
		FVector InCollisionBoxExtent,
		FName InDamageLogType = NAME_None);

	UFUNCTION(BlueprintCallable, Category = "SlashWave")
	void ApplyImmediateHit(AActor* Target);

protected:
	virtual void BeginPlay() override;

	// ── 碰撞体（水平刀光截面）──────────────────────────────────────────────
	/** 扁平水平盒，默认 60x30x35（cm），子类可在构造函数里调整 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> CollisionBox;

	// ── 投射物移动 ──────────────────────────────────────────────────────────
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

	// ── 可配置参数 ──────────────────────────────────────────────────────────
	/** 生存时间（秒），到期后触发 BP_OnExpired 并销毁 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile")
	float Lifetime = 1.2f;

	/** 飞行速度（cm/s）*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile")
	float Speed = 1400.f;

	/** 最大飞行距离（cm）。<= 0 时使用 Lifetime。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile", meta = (ClampMin = "0.0"))
	float MaxDistance = 800.f;

	/** 最大命中目标数量。<= 0 表示不限数量，仍会对同一目标只命中一次。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile")
	int32 MaxHitCount = 0;

	/** 运行时可覆盖碰撞盒半径。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile")
	FVector CollisionBoxExtent = FVector(30.f, 60.f, 35.f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile")
	FName DamageLogType = TEXT("Rune_SlashWave");

	// ── Blueprint 表现层钩子 ────────────────────────────────────────────────
	/** 命中新目标时触发（在此播放粒子/音效/贴花）*/
	UFUNCTION(BlueprintImplementableEvent, Category = "VFX")
	void BP_OnHitEnemy(AActor* HitActor, FVector HitLocation);

	/** 生存时间结束时触发（在此播放消散特效）*/
	UFUNCTION(BlueprintImplementableEvent, Category = "VFX")
	void BP_OnExpired();

private:
	// 运行时数据（InitProjectile 填入）
	UPROPERTY()
	TObjectPtr<ACharacter> SourceCharacter;

	UPROPERTY()
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	float DamageMagnitude = 0.f;

	bool bProjectileInitialized = false;

	/** 已命中目标列表，用于穿透跳过 */
	TArray<TWeakObjectPtr<AActor>> HitActors;

	FTimerHandle LifetimeTimerHandle;

	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	    bool bFromSweep, const FHitResult& SweepHitResult);

	/** 对目标施加 DamageEffect（SetByCaller Attribute.ActDamage = DamageMagnitude）*/
	bool ApplyDamageTo(AActor* Target, const FVector& HitLocation);

	/** 生存时间到期处理 */
	void Expire();

	void RefreshLifetimeFromDistance();
};
