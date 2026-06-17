// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Character/YogCharacterBase.h"
#include "AbilitySystem/Attribute/EnemyAttributeSet.h"
#include "BehaviorTree/BlackboardData.h"
#include "BehaviorTree/BehaviorTree.h"

#include "Data/EnemyData.h"
#include "EnemyCharacterBase.generated.h"

class UBuffFlowComponent;
class UEnemyHealthDisplayComponent;
class UEnemyWeaponDefinition;
class UMontageVFXBindingComponent;
class UYogAbilitySystemComponent;

/**
 *
 */
UCLASS()
class DEVKIT_API AEnemyCharacterBase : public AYogCharacterBase
{
	GENERATED_BODY()
public:

	AEnemyCharacterBase(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// 关卡 Buff（RuneDA FlowAsset）直接在此组件上激活，无需背包
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "BuffFlow")
	TObjectPtr<UBuffFlowComponent> BuffFlowComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|Health Display")
	TObjectPtr<UEnemyHealthDisplayComponent> HealthDisplayComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VFX")
	TObjectPtr<UMontageVFXBindingComponent> MontageVFXBindingComponent;

	// Tutorial/practice enemies can opt out of room clear, wave, and kill-count progression.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Level")
	bool bCountsForLevelClear = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy|Death", meta = (ClampMin = "0.0", ForceUnits = "s"))
	float DeathDisappearDelayAfterAnimation = 0.15f;

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintCallable, Category = "Feature")
	virtual void Die() override;

	virtual bool IsAlive() const override;
	virtual float GetDeathDisappearDelayAfterAnimation(bool bHasDissolveCue) const override;

	void PostInitializeComponents() override;

	UFUNCTION(BlueprintCallable, Category = "Enemy|Weapon")
	void SetPendingEnemyWeaponDefinition(UEnemyWeaponDefinition* WeaponDefinition);

	UFUNCTION(BlueprintCallable, Category = "Enemy|Weapon")
	void ApplyEnemyWeaponDefinition(UEnemyWeaponDefinition* WeaponDefinition);

	UFUNCTION(BlueprintPure, Category = "Enemy|Weapon")
	UEnemyWeaponDefinition* GetEquippedEnemyWeaponDefinition() const { return EquippedEnemyWeaponDefinition.Get(); }

	void SetAIAttackRuntimeContext(const FEnemyAIAttackOption& AttackOption, AActor* TargetActor, float DistanceToTarget);
	bool ConsumeAIAttackRuntimeContext(FEnemyAIAttackRuntimeContext& OutContext);
	void ClearAIAttackRuntimeContext();

protected:
	UFUNCTION()
	void OnHealthChangedForDeath(float NewHealth);

	UFUNCTION()
	void OnReceivedDamageForAI(UYogAbilitySystemComponent* SourceASC, float Damage);
	
	UPROPERTY(BlueprintReadWrite)
	TObjectPtr<UEnemyAttributeSet> EnemyAttributeSet;

	UPROPERTY(Transient)
	FEnemyAIAttackRuntimeContext PendingAIAttackContext;

	UPROPERTY(Transient)
	TObjectPtr<UEnemyWeaponDefinition> PendingEnemyWeaponDefinition;

	UPROPERTY(Transient)
	TObjectPtr<UEnemyWeaponDefinition> EquippedEnemyWeaponDefinition;

	UPROPERTY(Transient)
	TArray<TObjectPtr<AActor>> SpawnedEnemyWeaponActors;


	friend UEnemyAttributeSet;
};
