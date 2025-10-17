// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "BehaviorTree/BehaviorTree.h"
#include "EnemyData.generated.h"

/**
 * 
 */
//USTRUCT(BlueprintType)
//struct FEnemyCharacterData : public FTableRowBase
//{
//	GENERATED_BODY()
//
//public:
//	FEnemyCharacterData()
//		: AttackPower(0), AttackSpeed(0), AttackRange(0), CrticalRate(0), Crit_Damage(0)
//	{
//	};
//	UPROPERTY(EditAnywhere, BlueprintReadWrite)
//	FString Name;
//
//	UPROPERTY(EditAnywhere, BlueprintReadWrite)
//	float AttackPower = 0;
//
//	UPROPERTY(EditAnywhere, BlueprintReadWrite)
//	float AttackSpeed = 1;
//
//	UPROPERTY(EditAnywhere, BlueprintReadWrite)
//	float AttackRange = 1;
//
//	UPROPERTY(EditAnywhere, BlueprintReadWrite)
//	float CrticalRate = 1;
//
//	UPROPERTY(EditAnywhere, BlueprintReadWrite)
//	float Crit_Damage = 1;
//
//	UPROPERTY(EditAnywhere, BlueprintReadWrite)
//	TArray<UYogGameplayAbility*> Actions;
//
//};


UCLASS()
class DEVKIT_API UEnemyData : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UBehaviorTree> EnemyBT;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (RowType = "ActionData"))
	TArray<FDataTableRowHandle> ActionRows;




};
