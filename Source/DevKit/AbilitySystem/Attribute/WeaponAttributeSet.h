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
class UWeaponData;




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
    FGameplayAttributeData WeaponAtkPower;
    ATTRIBUTE_ACCESSORS(UWeaponAttributeSet, WeaponAtkPower);



    UPROPERTY(BlueprintReadWrite, Category = "Weapon|Player")
    FGameplayAttributeData WeaponAtkRange;
    ATTRIBUTE_ACCESSORS(UWeaponAttributeSet, WeaponAtkRange);


    UPROPERTY(BlueprintReadWrite, Category = "Weapon|Player")
    FGameplayAttributeData Weapon_CritRate;
    ATTRIBUTE_ACCESSORS(UWeaponAttributeSet, Weapon_CritRate);

    UPROPERTY(BlueprintReadWrite, Category = "Weapon|Player")
    FGameplayAttributeData Weapon_CritDmg;
    ATTRIBUTE_ACCESSORS(UWeaponAttributeSet, Weapon_CritDmg);


    //const FMovementData& moveData = CharacterData->GetMovementData();
    //const FYogBaseData& characterData = CharacterData->GetCharacterData();


};
