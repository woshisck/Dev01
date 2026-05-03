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

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintCallable, Category = "Feature")
	virtual void Die() override;

	virtual bool IsAlive() const override;

	void PostInitializeComponents() override;

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


	friend UEnemyAttributeSet;
};
