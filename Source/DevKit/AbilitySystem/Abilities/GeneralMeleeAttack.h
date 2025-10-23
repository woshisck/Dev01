// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/YogGameplayAbility.h"
#include "GeneralMeleeAttack.generated.h"

/**
 * 
 */
UCLASS()
class DEVKIT_API UGeneralMeleeAttack : public UYogGameplayAbility
{
	GENERATED_BODY()
public:
	UGeneralMeleeAttack(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());



//UPROPERTY(EditAnywhere, BlueprintReadWrite)
//float ActDamage = 20;

//UPROPERTY(EditAnywhere, BlueprintReadWrite)
//float ActRange = 400;

//UPROPERTY(EditAnywhere, BlueprintReadWrite)
//float ActResilience = 20;

//UPROPERTY(EditAnywhere, BlueprintReadWrite)
//float ActDmgReduce = 0;

//UPROPERTY(EditAnywhere, BlueprintReadWrite)
//float ActRotateSpeed = 360;

//UPROPERTY(EditAnywhere, BlueprintReadWrite)
//float JumpFrameTime = 0.15;

//UPROPERTY(EditAnywhere, BlueprintReadWrite)
//float FreezeFrameTime = 0.15;

//UPROPERTY(EditAnywhere, BlueprintReadWrite)
//TObjectPtr<UAnimMontage> Montage;

////UPROPERTY(EditAnywhere, BlueprintReadWrite)
////TSubclassOf<UYogGameplayAbility> Ability;

//UPROPERTY(EditAnywhere, BlueprintReadWrite)
//TSubclassOf<UYogGameplayAbility> Ability_Template;

//UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (RowType = "HitBoxData"))
//TArray<FHitBoxData> hitbox;
};
