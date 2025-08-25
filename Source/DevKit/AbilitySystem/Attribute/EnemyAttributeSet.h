// Fill out your copyright notice in the Description page of Project Settings.

#pragma once



#include "CoreMinimal.h"
#include "BaseAttributeSet.h"
#include "AttributeSetMacro.h"
#include "AbilitySystemComponent.h"
#include "EnemyAttributeSet.generated.h"



UCLASS()
class DEVKIT_API UEnemyAttributeSet : public UBaseAttributeSet{
	GENERATED_BODY()


public:



	UEnemyAttributeSet();

public:
	UPROPERTY(BlueprintReadWrite, Category = "Enemy")
	FGameplayAttributeData DetectPlayer_Hear;
	ATTRIBUTE_ACCESSORS(UEnemyAttributeSet, DetectPlayer_Hear);

	UPROPERTY(BlueprintReadWrite, Category = "Enemy")
	FGameplayAttributeData DetectPlayer_Visual;
	ATTRIBUTE_ACCESSORS(UEnemyAttributeSet, DetectPlayer_Visual);

	UPROPERTY(BlueprintReadWrite, Category = "Enemy")
	FGameplayAttributeData AttackRange;
	ATTRIBUTE_ACCESSORS(UEnemyAttributeSet, AttackRange);

    UPROPERTY(BlueprintReadWrite, Category = "Enemy")
    FGameplayAttributeData DropExp;
	ATTRIBUTE_ACCESSORS(UEnemyAttributeSet, DropExp);

};
