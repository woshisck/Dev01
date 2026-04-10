// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "BehaviorTree/BehaviorTree.h"
#include "AbilitySystem/Abilities/YogGameplayAbility.h"
#include "Data/CharacterData.h"
#include "EnemyData.generated.h"

class AEnemyCharacterBase;


USTRUCT(BlueprintType)
struct FEnemyAbilityData : public FTableRowBase
{
	GENERATED_BODY()

public:


	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<UYogGameplayAbility> MobAbility;

};


UCLASS()
class DEVKIT_API UEnemyData : public UCharacterData
{
	GENERATED_BODY()

public:

	// 对应的敌人 Actor 类（刷怪系统用此类生成敌人）
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy")
	TSubclassOf<AEnemyCharacterBase> EnemyClass;

	// 难度预算消耗（普通怪建议 2-4，精英怪建议 6-10）
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy")
	int32 DifficultyScore = 3;

};
