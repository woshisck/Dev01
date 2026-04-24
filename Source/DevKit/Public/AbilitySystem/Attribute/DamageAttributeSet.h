// Fill out your copyright notice in the Description page of Project Settings.

#pragma once



#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AttributeSetMacro.h"
#include "AbilitySystemComponent.h"
#include "AttributeSetMacro.h"
#include "DamageAttributeSet.generated.h"






UCLASS()
class DEVKIT_API UDamageAttributeSet : public UAttributeSet{
	GENERATED_BODY()


public:


	UDamageAttributeSet();

	//////////////////////////////////////////////// Weapon Attribute ////////////////////////////////////////////////
	ATTRIBUTE_ACCESSORS(UDamageAttributeSet, DamagePhysical);
	ATTRIBUTE_ACCESSORS(UDamageAttributeSet, DamageMagic);
	ATTRIBUTE_ACCESSORS(UDamageAttributeSet, DamagePure);

public:

    ////////////////////////////////////////////////// Player Attribute ////////////////////////////////////////////////
    UPROPERTY(BlueprintReadWrite, Category = "Weapon|Player")
    FGameplayAttributeData DamagePhysical;

    UPROPERTY(BlueprintReadWrite, Category = "Weapon|Player")
    FGameplayAttributeData DamageMagic;

    UPROPERTY(BlueprintReadWrite, Category = "Weapon|Player")
    FGameplayAttributeData DamagePure;

    // ─── 状态效果伤害（绕过护甲吸收，由GEExec_PoisonDamage / GA_Wound等写入）─────
    /** 状态效果伤害：直接减少 Health，跳过护甲吸收。不重播 Ability.Event.Damaged（防止递归）*/
    UPROPERTY(BlueprintReadWrite, Category = "Buff")
    FGameplayAttributeData DamageBuff;
    ATTRIBUTE_ACCESSORS(UDamageAttributeSet, DamageBuff);

    virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;
};

