// Fill out your copyright notice in the Description page of Project Settings.

#pragma once



#include "CoreMinimal.h"
#include "AbilitySystem/Attribute/BaseAttributeSet.h"

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
class DEVKIT_API UPlayerAttributeSet : public UAttributeSet{
	GENERATED_BODY()


public:



	UPlayerAttributeSet();


	//ATTRIBUTE_ACCESSORS(UPlayerAttributeSet, OutRoundLifeTime);
    //ATTRIBUTE_ACCESSORS(UPlayerAttributeSet, MiscNum);


    
    UPROPERTY(BlueprintReadWrite, Category = "Attributes|Player")
    FGameplayAttributeData SkillCD;
    ATTRIBUTE_ACCESSORS(UPlayerAttributeSet, SkillCD);

    // ── 冲刺充能系统 ──────────────────────────────────────────────
    // 当前格数由 USkillChargeComponent 管理，此处只存符文可修改的配置属性。

    /** 最大可储存充能格数（符文可 Additive +1） */
    UPROPERTY(BlueprintReadWrite, Category = "Attributes|Dash")
    FGameplayAttributeData MaxDashCharge;
    ATTRIBUTE_ACCESSORS(UPlayerAttributeSet, MaxDashCharge);

    /** 每格充能回复间隔（秒），符文可 Multiplicative ×1.25 */
    UPROPERTY(BlueprintReadWrite, Category = "Attributes|Dash")
    FGameplayAttributeData DashCooldownDuration;
    ATTRIBUTE_ACCESSORS(UPlayerAttributeSet, DashCooldownDuration);

    virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;

    //UPROPERTY(BlueprintReadWrite, Category = "Attributes|Player")
    //FGameplayAttributeData MAX_PassiveGA;
    //ATTRIBUTE_ACCESSORS(UPlayerAttributeSet, MAX_PassiveGA);


    //UPROPERTY(BlueprintReadWrite, Category = "Attributes|Player")
    //FGameplayAttributeData MAX_OffensiveGA;
    //ATTRIBUTE_ACCESSORS(UPlayerAttributeSet, MAX_OffensiveGA);
    
    //UPROPERTY(BlueprintReadWrite, Category = "Attributes|Player")
    //FGameplayAttributeData DashCD;
    //ATTRIBUTE_ACCESSORS(UPlayerAttributeSet, DashCD);

    //UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_Sanity, Category = "Attributes|Player")
    //FGameplayAttributeData DashDist;
    //ATTRIBUTE_ACCESSORS(UPlayerAttributeSet, DashDist);




    //UPROPERTY(BlueprintReadWrite, Category = "Attributes|Player")
    //FGameplayAttributeData OutRoundLifeTime;

    //const FMovementData& moveData = CharacterData->GetMovementData();
    //const FYogBaseAttributeData& characterData = CharacterData->GetBaseAttributeData();


};
