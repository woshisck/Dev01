// Fill out your copyright notice in the Description page of Project Settings.

#pragma once



#include "CoreMinimal.h"
#include "BaseAttributeSet.h"
#include "AttributeSetMacro.h"
#include "AbilitySystemComponent.h"
#include "EnemyAttributeSet.generated.h"



UCLASS()
class DEVKIT_API UEnemyAttributeSet : public UAttributeSet{
	GENERATED_BODY()


public:



	UEnemyAttributeSet();

public:
	UPROPERTY(BlueprintReadWrite, Category = "Enemy")
	FGameplayAttributeData DetectPlayer_Sound;
	ATTRIBUTE_ACCESSORS(UEnemyAttributeSet, DetectPlayer_Sound);

	UPROPERTY(BlueprintReadWrite, Category = "Enemy")
	FGameplayAttributeData DetectPlayer_Visual;
	ATTRIBUTE_ACCESSORS(UEnemyAttributeSet, DetectPlayer_Visual);

    UPROPERTY(BlueprintReadWrite, Category = "Enemy")
    FGameplayAttributeData DropExp;
	ATTRIBUTE_ACCESSORS(UEnemyAttributeSet, DropExp);

};
