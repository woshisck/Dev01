// Fill out your copyright notice in the Description page of Project Settings.

#pragma once



#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "AttributeSetMacro.h"
#include "AdditionAttributeSet.generated.h"


UCLASS()
class DEVKIT_API UAdditionAttributeSet : public UAttributeSet{
	GENERATED_BODY()


public:



	UAdditionAttributeSet();

public:



    ////////////////////////////////////////////////// Player Attribute ////////////////////////////////////////////////
    UPROPERTY(BlueprintReadWrite, Category = "Add_Attribute|Player")
    FGameplayAttributeData Add_Attack;
    ATTRIBUTE_ACCESSORS(UAdditionAttributeSet, Add_Attack);

    UPROPERTY(BlueprintReadWrite, Category = "Add_Attribute|Player")
    FGameplayAttributeData Add_AttackPower;
    ATTRIBUTE_ACCESSORS(UAdditionAttributeSet, Add_AttackPower);

    UPROPERTY(BlueprintReadOnly, Category = "Add_Attribute|Health")
    FGameplayAttributeData Add_Health;
    ATTRIBUTE_ACCESSORS(UAdditionAttributeSet, Add_Health);

    UPROPERTY(BlueprintReadOnly, Category = "Add_Attribute|Health")
    FGameplayAttributeData Add_MaxHealth;
    ATTRIBUTE_ACCESSORS(UAdditionAttributeSet, Add_MaxHealth);

    UPROPERTY(BlueprintReadWrite, Category = "Add_Attribute|Player")
    FGameplayAttributeData Add_Shield;
    ATTRIBUTE_ACCESSORS(UAdditionAttributeSet, Add_Shield);

    UPROPERTY(BlueprintReadWrite, Category = "Add_Attribute|Player")
    FGameplayAttributeData Add_AttackSpeed;
    ATTRIBUTE_ACCESSORS(UAdditionAttributeSet, Add_AttackSpeed);

    UPROPERTY(BlueprintReadWrite, Category = "Add_Attribute|Player")
    FGameplayAttributeData Add_AttackRange;
    ATTRIBUTE_ACCESSORS(UAdditionAttributeSet, Add_AttackRange);

    UPROPERTY(BlueprintReadWrite, Category = "Add_Attribute|Player")
    FGameplayAttributeData Add_Sanity;
    ATTRIBUTE_ACCESSORS(UAdditionAttributeSet, Add_Sanity);

    UPROPERTY(BlueprintReadWrite, Category = "Add_Attribute|Player")
    FGameplayAttributeData Add_MoveSpeed;
    ATTRIBUTE_ACCESSORS(UAdditionAttributeSet, Add_MoveSpeed);

    UPROPERTY(BlueprintReadWrite, Category = "Add_Attribute|Player")
    FGameplayAttributeData Add_Dodge;
    ATTRIBUTE_ACCESSORS(UAdditionAttributeSet, Add_Dodge);


    UPROPERTY(BlueprintReadWrite, Category = "Add_Attribute|Player")
    FGameplayAttributeData Add_Resilience;
    ATTRIBUTE_ACCESSORS(UAdditionAttributeSet, Add_Resilience);
    

    UPROPERTY(BlueprintReadWrite, Category = "Add_Attribute|Player")
    FGameplayAttributeData Add_Resist;
    ATTRIBUTE_ACCESSORS(UAdditionAttributeSet, Add_Resist);

    UPROPERTY(BlueprintReadWrite, Category = "Add_Attribute|Player")
    FGameplayAttributeData Add_DmgTaken;
    ATTRIBUTE_ACCESSORS(UAdditionAttributeSet, Add_DmgTaken);

    UPROPERTY(BlueprintReadWrite, Category = "Add_Attribute|Player")
    FGameplayAttributeData Add_Crit_Rate;
    ATTRIBUTE_ACCESSORS(UAdditionAttributeSet, Add_Crit_Rate);


    UPROPERTY(BlueprintReadWrite, Category = "Add_Attribute|Player")
    FGameplayAttributeData Add_Crit_Damage;
    ATTRIBUTE_ACCESSORS(UAdditionAttributeSet, Add_Crit_Damage);


    //const FMovementData& moveData = CharacterData->GetMovementData();
    //const FYogBaseData& characterData = CharacterData->GetCharacterData();


};
