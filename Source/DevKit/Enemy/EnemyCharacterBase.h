// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Character/YogCharacterBase.h"
#include "AbilitySystem/Attribute/EnemyAttributeSet.h"
#include "BehaviorTree/BlackboardData.h"
#include "BehaviorTree/BehaviorTree.h"

#include "DevKit/Data/EnemyData.h"
#include "AbilitySystem/Abilities/GeneralMeleeAttack.h"
#include "EnemyCharacterBase.generated.h"


/**
 * 
 */
UCLASS()
class DEVKIT_API AEnemyCharacterBase : public AYogCharacterBase
{
	GENERATED_BODY()
public:

	AEnemyCharacterBase(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintCallable, Category = "Feature")
	virtual void Die() override;
	
	void PostInitializeComponents() override;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UEnemyAttributeSet> EnemyAttributeSet;

	UFUNCTION(BlueprintCallable)
	void SetupAI(UBehaviorTree* bt, UBlackboardData* bb);



	//UPROPERTY(EditAnywhere, BlueprintReadWrite)
	//TObjectPtr<UEnemyData> EnemyData;


	UFUNCTION(BlueprintCallable)
	void InitEnemyData(UEnemyData* enemy_data);

	UPROPERTY()
	TSubclassOf<UYogGameplayAbility> Ability_Class; // Or a more specific base class


	friend UEnemyAttributeSet;
};
