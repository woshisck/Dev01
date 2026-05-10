#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "BuffFlowProjectile.generated.h"

class UCurveFloat;
class UGameplayEffect;
class UProjectileMovementComponent;
class UCapsuleComponent;
class USceneComponent;
class ACharacter;

UENUM(BlueprintType)
enum class EBuffFlowProjectileTriggerMode : uint8
{
	HitOnce UMETA(DisplayName = "Hit Once"),
	PeriodicOverlap UMETA(DisplayName = "Periodic Overlap")
};

UENUM(BlueprintType)
enum class EBuffFlowProjectileCurveInput : uint8
{
	Constant UMETA(DisplayName = "Constant"),
	CreatorAttack UMETA(DisplayName = "Creator Attack"),
	CreatorAttackPower UMETA(DisplayName = "Creator Attack Power"),
	CreatorHealth UMETA(DisplayName = "Creator Health"),
	CreatorMaxHealth UMETA(DisplayName = "Creator Max Health"),
	CreatorArmor UMETA(DisplayName = "Creator Armor"),
	CreatorMaxArmor UMETA(DisplayName = "Creator Max Armor"),
	CreatorMoveSpeed UMETA(DisplayName = "Creator Move Speed")
};

UENUM(BlueprintType)
enum class EBuffFlowProjectileSpeedCurveMode : uint8
{
	AbsoluteSpeed UMETA(DisplayName = "Absolute Speed"),
	SpeedMultiplier UMETA(DisplayName = "Speed Multiplier")
};

USTRUCT(BlueprintType)
struct DEVKIT_API FBuffFlowProjectileAttributeSnapshot
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "BuffFlow|Projectile")
	float Attack = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "BuffFlow|Projectile")
	float AttackPower = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "BuffFlow|Projectile")
	float Health = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "BuffFlow|Projectile")
	float MaxHealth = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "BuffFlow|Projectile")
	float Armor = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "BuffFlow|Projectile")
	float MaxArmor = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "BuffFlow|Projectile")
	float MoveSpeed = 0.f;
};

USTRUCT(BlueprintType)
struct DEVKIT_API FBuffFlowProjectileRuntimeConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BuffFlow|Projectile")
	EBuffFlowProjectileTriggerMode TriggerMode = EBuffFlowProjectileTriggerMode::HitOnce;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BuffFlow|Projectile", meta = (ClampMin = "0.01", EditCondition = "TriggerMode == EBuffFlowProjectileTriggerMode::PeriodicOverlap", EditConditionHides))
	float TriggerInterval = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BuffFlow|Projectile", meta = (ClampMin = "0.0"))
	float Lifetime = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BuffFlow|Projectile")
	TObjectPtr<UCurveFloat> LifetimeCurve = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BuffFlow|Projectile", meta = (EditCondition = "LifetimeCurve != nullptr", EditConditionHides))
	EBuffFlowProjectileCurveInput LifetimeCurveInput = EBuffFlowProjectileCurveInput::Constant;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BuffFlow|Projectile", meta = (EditCondition = "LifetimeCurve != nullptr", EditConditionHides))
	float LifetimeCurveConstantInput = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BuffFlow|Projectile", meta = (ClampMin = "0.0"))
	float Speed = 1200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BuffFlow|Projectile")
	TObjectPtr<UCurveFloat> SpeedOverLifeCurve = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BuffFlow|Projectile", meta = (EditCondition = "SpeedOverLifeCurve != nullptr", EditConditionHides))
	EBuffFlowProjectileSpeedCurveMode SpeedCurveMode = EBuffFlowProjectileSpeedCurveMode::AbsoluteSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BuffFlow|Projectile", meta = (ClampMin = "1.0"))
	float CollisionCapsuleRadius = 24.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BuffFlow|Projectile", meta = (ClampMin = "1.0"))
	float CollisionCapsuleHalfHeight = 48.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BuffFlow|Projectile")
	bool bDestroyOnHitTrigger = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BuffFlow|Projectile")
	bool bDestroyOnWorldStaticHit = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BuffFlow|Effect")
	TSubclassOf<UGameplayEffect> EffectClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BuffFlow|Effect")
	bool bApplyPureDamageFallback = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BuffFlow|Effect")
	FGameplayTag SetByCallerMagnitudeTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BuffFlow|Effect")
	float BaseEffectMagnitude = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BuffFlow|Effect")
	float CreatorAttackMagnitudeScale = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BuffFlow|Effect")
	float CreatorAttackPowerMagnitudeScale = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BuffFlow|Effect", meta = (ClampMin = "0.0"))
	float EffectRadius = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BuffFlow|Effect", meta = (ClampMin = "0"))
	int32 MaxTriggersPerTarget = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BuffFlow|Events")
	FGameplayTag TriggerGameplayEventTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BuffFlow|Events")
	bool bSendTriggerEventToCreator = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BuffFlow|Events")
	FGameplayTag ExpireGameplayEventTag;
};

UCLASS(BlueprintType, Blueprintable)
class DEVKIT_API ABuffFlowProjectile : public AActor
{
	GENERATED_BODY()

public:
	ABuffFlowProjectile();

	UFUNCTION(BlueprintCallable, Category = "BuffFlow|Projectile")
	void SetCreatorForSpawn(AActor* InCreator);

	UFUNCTION(BlueprintCallable, Category = "BuffFlow|Projectile")
	void InitBuffFlowProjectile(AActor* InCreator, const FBuffFlowProjectileRuntimeConfig& InConfig);

	UFUNCTION(BlueprintPure, Category = "BuffFlow|Projectile")
	AActor* GetCreatorActor() const { return CreatorActor.Get(); }

	UFUNCTION(BlueprintPure, Category = "BuffFlow|Projectile")
	const FBuffFlowProjectileAttributeSnapshot& GetCreatorAttributeSnapshot() const { return CreatorAttributes; }

	UFUNCTION(BlueprintPure, Category = "BuffFlow|Projectile")
	float GetEffectMagnitude() const { return EffectMagnitude; }

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UCapsuleComponent> CollisionCapsule;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

	UFUNCTION(BlueprintImplementableEvent, Category = "BuffFlow|Projectile")
	void BP_OnExpired();

	UFUNCTION(BlueprintImplementableEvent, Category = "BuffFlow|Projectile", meta = (DisplayName = "On Enemy Character Hit"))
	void BP_OnEnemyCharacterHit(ACharacter* HitCharacter, FVector HitLocation, float Magnitude);

	UFUNCTION(BlueprintImplementableEvent, Category = "BuffFlow|Projectile")
	void BP_OnWorldHit(FVector HitLocation);

private:
	UPROPERTY()
	TWeakObjectPtr<AActor> CreatorActor;

	UPROPERTY()
	FBuffFlowProjectileRuntimeConfig RuntimeConfig;

	UPROPERTY()
	FBuffFlowProjectileAttributeSnapshot CreatorAttributes;

	UPROPERTY()
	TArray<TWeakObjectPtr<AActor>> OverlappingTargets;

	TMap<TWeakObjectPtr<AActor>, int32> TriggerCountsByActor;
	FTimerHandle LifetimeTimerHandle;
	FTimerHandle PeriodicTriggerTimerHandle;
	float ElapsedSeconds = 0.f;
	float ResolvedLifetime = 1.f;
	float ResolvedBaseSpeed = 1200.f;
	float EffectMagnitude = 0.f;
	bool bInitialized = false;
	bool bInitialOverlapCheckScheduled = false;

	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepHitResult);

	UFUNCTION()
	void OnOverlapEnd(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	void SnapshotCreatorAttributes();
	float GetCurveInputValue(EBuffFlowProjectileCurveInput Input, float ConstantInput) const;
	float ResolveLifetime() const;
	float ResolveSpeed() const;
	void ApplyResolvedSpeed();
	void StartRuntimeTimers();
	void ScheduleInitialOverlapCheck();
	void HandleInitialOverlaps();
	void HandlePeriodicTrigger();
	void TriggerEffectAt(AActor* DirectTarget, const FVector& TriggerLocation);
	void TriggerEffectForTargets(const TArray<AActor*>& Targets, const FVector& TriggerLocation);
	bool ApplyEffectToTarget(AActor* Target, const FVector& TriggerLocation);
	void SendTriggerGameplayEvent(AActor* Target, const FVector& TriggerLocation) const;
	void SendExpireGameplayEvent() const;
	bool CanTriggerTarget(AActor* Target) const;
	void AddOverlappingTarget(AActor* Target);
	void RemoveOverlappingTarget(AActor* Target);
	void Expire();
};
