// Fill out your copyright notice in the Description page of Project Settings.

#pragma once



#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "AttributeSetMacro.h"
#include "BaseAttributeSet.generated.h"


class AActor;
class UYogAbilitySystemComponent;
class UObject;
class UWorld;




////////////////////////////////////////////////// Player Attribute ////////////////////////////////////////////////
//float Attack = 0;
//float AttackPower = 1;
//float MiscNum = 1;
//float SkillCD = 1;
//float MAX_PassiveGA = 1;
//float MAX_OffensiveGA = 1;
//float MaxHealth = 30;
//float OutRoundLifeTime = 0;
//float MoveSpeed = 6;
//float Dash = 1;
//float DashCD = 1;
//float DashDist = 4;
//float Dodge = 0;
//float Resilience = 0;
//float Resist = 0;
//float Shield = 0;


////////////////////////////////////////////////// Ability Attribute ////////////////////////////////////////////////
//float ActDamage = 20;
//float AbilityRange = 400;
//float AbilityResilience = 20;
//float AbilityDmgReduce = 0;
//float ActRotateSpeed = 360;
//float JumpFrameTime = 0.15;
//float FreezeFrameTime = 0.15;




UCLASS()
class DEVKIT_API UBaseAttributeSet : public UAttributeSet{
	GENERATED_BODY()


public:



	UBaseAttributeSet();

	UYogAbilitySystemComponent* GetASC() const;


    virtual void PreAttributeBaseChange(const FGameplayAttribute& attribute, float& newValue) const override;
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;


protected:
	void AdjustAttributeForMaxChange(FGameplayAttributeData& AffectedAttribute, const FGameplayAttributeData& MaxAttribute, float NewMaxValue, const FGameplayAttribute& AffectedAttributeProperty);

public:



    ////////////////////////////////////////////////// Player Attribute ////////////////////////////////////////////////
    UPROPERTY(BlueprintReadWrite, Category = "Attributes|Player")
    FGameplayAttributeData Attack;
    ATTRIBUTE_ACCESSORS(UBaseAttributeSet, Attack);

    UPROPERTY(BlueprintReadWrite, Category = "Attributes|Player")
    FGameplayAttributeData AttackPower;
    ATTRIBUTE_ACCESSORS(UBaseAttributeSet, AttackPower);

    UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Health, Category = "Health")
    FGameplayAttributeData Health;
    ATTRIBUTE_ACCESSORS(UBaseAttributeSet, Health);

    UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxHealth, Category = "Health")
    FGameplayAttributeData MaxHealth;
    ATTRIBUTE_ACCESSORS(UBaseAttributeSet, MaxHealth);

    UPROPERTY(BlueprintReadWrite, Category = "Attributes|Player")
    FGameplayAttributeData Shield;
    ATTRIBUTE_ACCESSORS(UBaseAttributeSet, Shield);


    UPROPERTY(BlueprintReadWrite, Category = "Attributes|Player")
    FGameplayAttributeData Sanity;
    ATTRIBUTE_ACCESSORS(UBaseAttributeSet, Sanity);

    UPROPERTY(BlueprintReadWrite, Category = "Attributes|Player")
    FGameplayAttributeData MoveSpeed;
    ATTRIBUTE_ACCESSORS(UBaseAttributeSet, MoveSpeed);


    UPROPERTY(BlueprintReadWrite, Category = "Attributes|Player")
    FGameplayAttributeData Dodge;
    ATTRIBUTE_ACCESSORS(UBaseAttributeSet, Dodge);


    UPROPERTY(BlueprintReadWrite, Category = "Attributes|Player")
    FGameplayAttributeData Resilience;
    ATTRIBUTE_ACCESSORS(UBaseAttributeSet, Resilience);
    

    UPROPERTY(BlueprintReadWrite, Category = "Attributes|Player")
    FGameplayAttributeData Resist;
    ATTRIBUTE_ACCESSORS(UBaseAttributeSet, Resist);

    UPROPERTY(BlueprintReadWrite, Category = "Attributes|Player")
    FGameplayAttributeData DmgTaken;
    ATTRIBUTE_ACCESSORS(UBaseAttributeSet, DmgTaken);

    UPROPERTY(BlueprintReadWrite, Category = "Attributes|Player")
    FGameplayAttributeData Crit_Rate;
    ATTRIBUTE_ACCESSORS(UBaseAttributeSet, Crit_Rate);


    UPROPERTY(BlueprintReadWrite, Category = "Attributes|Player")
    FGameplayAttributeData Crit_Damage;
    ATTRIBUTE_ACCESSORS(UBaseAttributeSet, Crit_Damage);



    UFUNCTION()
    void Init(UCharacterData* data);


	UFUNCTION()
	void OnRep_Health(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_MaxHealth(const FGameplayAttributeData& OldValue);


    //const FMovementData& moveData = CharacterData->GetMovementData();
    //const FYogBaseData& characterData = CharacterData->GetCharacterData();


};
