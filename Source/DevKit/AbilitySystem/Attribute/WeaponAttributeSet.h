// Fill out your copyright notice in the Description page of Project Settings.

#pragma once



#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "AttributeSetMacro.h"
#include "WeaponAttributeSet.generated.h"


class AActor;
class UYogAbilitySystemComponent;
class UObject;
class UWorld;
struct FGameplayEffectSpec;





UCLASS()
class DEVKIT_API UWeaponAttributeSet : public UAttributeSet{

	GENERATED_BODY()


public:



	UWeaponAttributeSet();

	//////////////////////////////////////////////// Weapon Attribute ////////////////////////////////////////////////


public:



    ////////////////////////////////////////////////// Player Attribute ////////////////////////////////////////////////
    UPROPERTY(BlueprintReadWrite, Category = "Weapon|Player")
    FGameplayAttributeData WeaponAtk;
    ATTRIBUTE_ACCESSORS(UWeaponAttributeSet, WeaponAtk);


    UPROPERTY(BlueprintReadWrite, Category = "Weapon|Player")
    FGameplayAttributeData WeaponAtkSpeed;
    ATTRIBUTE_ACCESSORS(UWeaponAttributeSet, WeaponAtkSpeed);


    UPROPERTY(BlueprintReadWrite, Category = "Weapon|Player")
    FGameplayAttributeData WeaponRange;
    ATTRIBUTE_ACCESSORS(UWeaponAttributeSet, WeaponRange);

    //const FMovementData& moveData = CharacterData->GetMovementData();
    //const FYogCharacterData& characterData = CharacterData->GetCharacterData();


};
