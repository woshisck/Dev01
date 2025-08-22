// Fill out your copyright notice in the Description page of Project Settings.

#pragma once



#include "CoreMinimal.h"
#include "BaseAttributeSet.h"

#include "AttributeSetMacro.h"
#include "AbilitySystemComponent.h"
#include "PlayerAttributeSet.generated.h"


class AActor;
class UYogAbilitySystemComponent;
class UObject;
class UWorld;
struct FGameplayEffectSpec;



#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)




UCLASS()
class DEVKIT_API UPlayerAttributeSet : public UBaseAttributeSet{
	GENERATED_BODY()


public:



	UPlayerAttributeSet();









	//ATTRIBUTE_ACCESSORS(UPlayerAttributeSet, OutRoundLifeTime);
    //ATTRIBUTE_ACCESSORS(UPlayerAttributeSet, MiscNum);


    
    UPROPERTY(BlueprintReadWrite, Category = "Attributes|Player")
    FGameplayAttributeData SkillCD;
    ATTRIBUTE_ACCESSORS(UPlayerAttributeSet, SkillCD);
    
    UPROPERTY(BlueprintReadWrite, Category = "Attributes|Player")
    FGameplayAttributeData MAX_PassiveGA;
    ATTRIBUTE_ACCESSORS(UPlayerAttributeSet, MAX_PassiveGA);


    UPROPERTY(BlueprintReadWrite, Category = "Attributes|Player")
    FGameplayAttributeData MAX_OffensiveGA;
    ATTRIBUTE_ACCESSORS(UPlayerAttributeSet, MAX_OffensiveGA);
    
    UPROPERTY(BlueprintReadWrite, Category = "Attributes|Player")
    FGameplayAttributeData DashCD;
    ATTRIBUTE_ACCESSORS(UPlayerAttributeSet, DashCD);


    UPROPERTY(BlueprintReadWrite, Category = "Attributes|Player")
    FGameplayAttributeData DashDist;
    ATTRIBUTE_ACCESSORS(UPlayerAttributeSet, DashDist);


    //UPROPERTY(BlueprintReadWrite, Category = "Attributes|Player")
    //FGameplayAttributeData OutRoundLifeTime;

    //const FMovementData& moveData = CharacterData->GetMovementData();
    //const FYogCharacterData& characterData = CharacterData->GetCharacterData();


};
