#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "SlashWaveProjectile.generated.h"

class UBoxComponent;
class UProjectileMovementComponent;
class UAbilitySystemComponent;
class UGameplayEffect;
class UNiagaraComponent;
class UNiagaraSystem;
class ACharacter;

USTRUCT(BlueprintType)
struct DEVKIT_API FSlashWaveProjectileRuntimeConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slash Wave")
	float Damage = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slash Wave")
	TSubclassOf<UGameplayEffect> DamageEffect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slash Wave", meta = (ClampMin = "1.0"))
	float Speed = 1400.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slash Wave", meta = (ClampMin = "0.0"))
	float MaxDistance = 800.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slash Wave")
	int32 MaxHitCount = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slash Wave", meta = (ClampMin = "1", ClampMax = "20"))
	int32 DamageApplicationsPerTarget = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slash Wave", meta = (ClampMin = "0.0"))
	float DamageApplicationInterval = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slash Wave")
	FVector CollisionBoxExtent = FVector(30.f, 60.f, 35.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slash Wave|Visual")
	bool bScaleVisualWithCollisionExtent = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slash Wave|Visual")
	FVector VisualScaleMultiplier = FVector(1.f, 1.f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slash Wave|Visual")
	TObjectPtr<UNiagaraSystem> ProjectileVisualNiagaraSystem = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slash Wave|Visual")
	FVector ProjectileVisualNiagaraScale = FVector(1.f, 1.f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slash Wave|Visual")
	bool bHideDefaultProjectileVisuals = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slash Wave|Visual")
	TObjectPtr<UNiagaraSystem> HitNiagaraSystem = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slash Wave|Visual")
	FVector HitNiagaraScale = FVector(1.f, 1.f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slash Wave|Visual")
	TObjectPtr<UNiagaraSystem> ExpireNiagaraSystem = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slash Wave|Visual")
	FVector ExpireNiagaraScale = FVector(1.f, 1.f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slash Wave|Events")
	FGameplayTag HitGameplayEventTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slash Wave|Events")
	FGameplayTag ExpireGameplayEventTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slash Wave")
	FName DamageLogType = TEXT("Rune_SlashWave");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slash Wave|Collision")
	bool bDestroyOnWorldStaticHit = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slash Wave|Damage")
	bool bForcePureDamage = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slash Wave|Damage", meta = (ClampMin = "0.0"))
	float BonusArmorDamageMultiplier = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slash Wave|Damage")
	TSubclassOf<UGameplayEffect> AdditionalHitEffect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slash Wave|Damage")
	FGameplayTag AdditionalHitSetByCallerTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slash Wave|Damage")
	float AdditionalHitSetByCallerValue = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slash Wave|Split")
	bool bSplitOnFirstHit = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slash Wave|Split", meta = (ClampMin = "0"))
	int32 ProjectileGeneration = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slash Wave|Split", meta = (ClampMin = "0"))
	int32 MaxSplitGenerations = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slash Wave|Split", meta = (ClampMin = "1"))
	int32 SplitProjectileCount = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slash Wave|Split", meta = (ClampMin = "0.0", ClampMax = "180.0"))
	float SplitConeAngleDegrees = 45.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slash Wave|Split")
	bool bRandomizeSplitDirections = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slash Wave|Split", meta = (ClampMin = "0.0", ClampMax = "180.0", EditCondition = "bRandomizeSplitDirections"))
	float SplitRandomYawJitterDegrees = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slash Wave|Split", meta = (ClampMin = "0.0", ClampMax = "45.0", EditCondition = "bRandomizeSplitDirections"))
	float SplitRandomPitchDegrees = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slash Wave|Split", meta = (ClampMin = "0.0"))
	float SplitDamageMultiplier = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slash Wave|Split", meta = (ClampMin = "0.01"))
	float SplitSpeedMultiplier = 2.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slash Wave|Split", meta = (ClampMin = "0.0"))
	float SplitMaxDistanceMultiplier = 0.6f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slash Wave|Split")
	FVector SplitCollisionBoxExtentMultiplier = FVector(0.5f, 0.5f, 0.5f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slash Wave|Split|Bounce")
	bool bBounceOnEnemyHit = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slash Wave|Split|Bounce", meta = (ClampMin = "0"))
	int32 MaxEnemyBounces = 0;
};

struct FSlashWaveHitRecord
{
	TWeakObjectPtr<AActor> Actor;
	int32 AppliedCount = 0;
	FTimerHandle RepeatTimerHandle;
};

/**
 * 刀光投射物 — 水平飞行，穿透多个敌人，Timer 到期后销毁。
 *
 * 工作流：
 *   1. GA_SlashWaveCounter / BuffFlow deferred-spawns ASlashWaveProjectile, sets SourceCharacter, then initializes it.
 *   2. 投射物沿发射方向飞行，CollisionBox 与 Pawn 发生 Overlap
 *   3. 每次命中新目标 → 施加 DamageEffect（SetByCaller Attribute.ActDamage）
 *   4. 已触发目标按 DamageApplicationsPerTarget 控制重复伤害次数
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
	void InitProjectileWithConfig(ACharacter* InSource, const FSlashWaveProjectileRuntimeConfig& InConfig);

	UFUNCTION(BlueprintCallable, Category = "SlashWave")
	void SetSourceCharacterForSpawn(ACharacter* InSource);

	UFUNCTION(BlueprintPure, Category = "SlashWave")
	ACharacter* GetSourceCharacter() const;

	UFUNCTION(BlueprintPure, Category = "SlashWave")
	AActor* GetCreatorActor() const;

	UFUNCTION(BlueprintCallable, Category = "SlashWave")
	void InitProjectileAdvanced(
		ACharacter* InSource,
		float InDamage,
		TSubclassOf<UGameplayEffect> InDamageEffect,
		float InSpeed,
		float InMaxDistance,
		int32 InMaxHitCount,
		int32 InDamageApplicationsPerTarget,
		float InDamageApplicationInterval,
		FVector InCollisionBoxExtent,
		FName InDamageLogType = NAME_None,
		bool bInScaleVisualWithCollisionExtent = true,
		FVector InVisualScaleMultiplier = FVector(1.f, 1.f, 1.f));

	UFUNCTION(BlueprintCallable, Category = "SlashWave")
	void ApplyImmediateHit(AActor* Target);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

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

	/** 最大命中目标数量。<= 0 表示不限数量。每个目标可按 DamageApplicationsPerTarget 受到多次伤害。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile")
	int32 MaxHitCount = 0;

	/** 同一目标可被同一道刀光伤害的次数。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile", meta = (ClampMin = "1", ClampMax = "20"))
	int32 DamageApplicationsPerTarget = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile", meta = (ClampMin = "0.0"))
	float DamageApplicationInterval = 0.25f;

	/** 运行时可覆盖碰撞盒半径。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile")
	FVector CollisionBoxExtent = FVector(30.f, 60.f, 35.f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile")
	FVector VisualScaleMultiplier = FVector(1.f, 1.f, 1.f);

	UPROPERTY()
	TObjectPtr<UNiagaraComponent> RuntimeVisualNiagaraComponent;

	UPROPERTY()
	TObjectPtr<UNiagaraSystem> RuntimeVisualNiagaraSystem;

	UPROPERTY()
	FVector RuntimeVisualNiagaraScale = FVector(1.f, 1.f, 1.f);

	UPROPERTY()
	bool bRuntimeHideDefaultProjectileVisuals = false;

	UPROPERTY()
	TObjectPtr<UNiagaraSystem> RuntimeHitNiagaraSystem;

	UPROPERTY()
	FVector RuntimeHitNiagaraScale = FVector(1.f, 1.f, 1.f);

	UPROPERTY()
	TObjectPtr<UNiagaraSystem> RuntimeExpireNiagaraSystem;

	UPROPERTY()
	FVector RuntimeExpireNiagaraScale = FVector(1.f, 1.f, 1.f);

	UPROPERTY()
	FGameplayTag RuntimeHitGameplayEventTag;

	UPROPERTY()
	FGameplayTag RuntimeExpireGameplayEventTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile")
	FName DamageLogType = TEXT("Rune_SlashWave");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile")
	bool bDestroyOnWorldStaticHit = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile")
	bool bForcePureDamage = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile")
	float BonusArmorDamageMultiplier = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile")
	TSubclassOf<UGameplayEffect> AdditionalHitEffectClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile")
	FGameplayTag AdditionalHitSetByCallerTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile")
	float AdditionalHitSetByCallerValue = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile")
	bool bSplitOnFirstHit = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile")
	int32 ProjectileGeneration = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile")
	int32 MaxSplitGenerations = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile")
	int32 SplitProjectileCount = 3;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile")
	float SplitConeAngleDegrees = 45.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile")
	bool bRandomizeSplitDirections = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile")
	float SplitRandomYawJitterDegrees = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile")
	float SplitRandomPitchDegrees = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile")
	float SplitDamageMultiplier = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile")
	float SplitSpeedMultiplier = 2.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile")
	float SplitMaxDistanceMultiplier = 0.6f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile")
	FVector SplitCollisionBoxExtentMultiplier = FVector(0.5f, 0.5f, 0.5f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile")
	bool bBounceOnEnemyHit = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile")
	int32 MaxEnemyBounces = 0;

	// ── Blueprint 表现层钩子 ────────────────────────────────────────────────
	/** 命中新目标时触发（在此播放粒子/音效/贴花）*/
	UFUNCTION(BlueprintImplementableEvent, Category = "VFX")
	void BP_OnHitEnemy(AActor* HitActor, FVector HitLocation);

	/** 生存时间结束时触发（在此播放消散特效）*/
	UFUNCTION(BlueprintImplementableEvent, Category = "VFX")
	void BP_OnExpired();

private:
	// 运行时数据（InitProjectile 填入）
	UPROPERTY(BlueprintReadOnly, Category = "SlashWave", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<ACharacter> SourceCharacter;

	UPROPERTY()
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	float DamageMagnitude = 0.f;

	bool bProjectileInitialized = false;

	/** 已触发过的目标，用于限制目标数和重复伤害次数。 */
	TArray<FSlashWaveHitRecord> HitRecords;

	bool bHasSplit = false;
	bool bInitialOverlapCheckScheduled = false;
	int32 EnemyBounceCount = 0;

	FTimerHandle LifetimeTimerHandle;

	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	    bool bFromSweep, const FHitResult& SweepHitResult);

	/** 对目标施加 DamageEffect（SetByCaller Attribute.ActDamage = DamageMagnitude）*/
	bool ApplyDamageTo(AActor* Target, const FVector& HitLocation);

	bool TryStartDamageSequence(AActor* Target, const FVector& HitLocation, const FHitResult* HitResult = nullptr);
	void ScheduleInitialOverlapCheck();
	void HandleInitialOverlaps();
	void ApplyDamageTickForRecord(int32 RecordIndex);
	int32 FindHitRecordIndex(AActor* Target) const;
	void ClearRepeatTimers();
	void ApplyBonusArmorDamageTo(AActor* Target, UAbilitySystemComponent* TargetASC) const;
	void ApplyAdditionalHitEffectTo(AActor* Target, UAbilitySystemComponent* SourceASC, UAbilitySystemComponent* TargetASC);
	void SendHitGameplayEvent(AActor* Target) const;
	void SendExpireGameplayEvent() const;
	void TrySplitFromImpact(AActor* ImpactActor, const FVector& ImpactLocation);
	void TryBounceFromEnemyHit(AActor* ImpactActor, const FVector& HitLocation, const FHitResult* HitResult);
	ACharacter* ResolveFallbackSourceCharacter() const;
	void CacheSourceCharacterFromSpawnReferences();

	/** 生存时间到期处理 */
	void Expire();

	void RefreshLifetimeFromDistance();
	void ApplyRuntimeVisualConfig(const FSlashWaveProjectileRuntimeConfig& InConfig);
	void HideDefaultProjectileVisualComponents();
};
