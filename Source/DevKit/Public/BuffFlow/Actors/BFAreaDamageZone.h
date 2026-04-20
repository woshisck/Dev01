#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayEffect.h"
#include "BFAreaDamageZone.generated.h"

class USphereComponent;
class ACharacter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAreaDamageExpired);

/**
 * 范围伤害区域 Actor —— 由 BFNode_AreaDamage 生成。
 *
 * 工作流：
 *   1. BFNode_AreaDamage 在延迟后 SpawnActor，随即调用 InitZone
 *   2. USphereComponent 与 Pawn Overlap，DamageTick 定时遍历区域内敌人施加 GE
 *   3. 持续时间结束 → BP_OnExpired → Destroy
 *
 * 表现层（粒子/贴花/音效）由 Blueprint 子类实现 BP_OnActivate / BP_OnDamageTick / BP_OnExpired。
 */
UCLASS(BlueprintType, Blueprintable)
class DEVKIT_API ABFAreaDamageZone : public AActor
{
	GENERATED_BODY()

public:
	ABFAreaDamageZone();

	UFUNCTION(BlueprintCallable, Category = "AreaDamage")
	void InitZone(ACharacter* InSource, float InRadius, float InDuration,
	              float InDamageAmount, float InDamageInterval,
	              TSubclassOf<UGameplayEffect> InDamageEffect);

	UPROPERTY(BlueprintAssignable, Category = "AreaDamage")
	FOnAreaDamageExpired OnExpiredDelegate;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USphereComponent> CollisionSphere;

	UFUNCTION(BlueprintImplementableEvent, Category = "VFX")
	void BP_OnActivate();

	UFUNCTION(BlueprintImplementableEvent, Category = "VFX")
	void BP_OnDamageTick();

	UFUNCTION(BlueprintImplementableEvent, Category = "VFX")
	void BP_OnExpired();

private:
	UPROPERTY()
	TObjectPtr<ACharacter> SourceCharacter;

	UPROPERTY()
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	float DamageMagnitude  = 0.f;
	float DamageInterval   = 1.f;
	float Duration         = 5.f;

	FTimerHandle LifetimeTimerHandle;
	FTimerHandle DamageTickTimerHandle;

	void DamageTick();
	void ApplyDamageTo(AActor* Target);
	void Expire();
};
