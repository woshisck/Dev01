// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "BehaviorTree/BehaviorTree.h"
#include "DevKit/AbilitySystem/Abilities/YogGameplayAbility.h"
#include "EnemyData.generated.h"



USTRUCT(BlueprintType)
struct FEnemyAbilityData : public FTableRowBase
{
	GENERATED_BODY()

public:


	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<UYogGameplayAbility> MobAbility;

};


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
