// Fill out your copyright notice in the Description page of Project Settings.

#pragma once



#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "BaseAttributeSet.generated.h"


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


////////////////////////////////////////////////// Weapon Attribute ////////////////////////////////////////////////
//float WeaponAtk = 0;
//float WeaponAtkSpeed = 1;
//float WeaponRange = 1;
//float CrticalRate = 0;
//float CriticalDamage = 1;



UCLASS()
class DEVKIT_API UBaseAttributeSet : public UAttributeSet{
	GENERATED_BODY()


public:



	UBaseAttributeSet();

	UWorld* GetWorld() const override;

	UYogAbilitySystemComponent* GetASC() const;


	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;


	//////////////////////////////////////////////// Player Attribute ////////////////////////////////////////////////
	ATTRIBUTE_ACCESSORS(UBaseAttributeSet, Attack);
	ATTRIBUTE_ACCESSORS(UBaseAttributeSet, AttackPower);

	ATTRIBUTE_ACCESSORS(UBaseAttributeSet, MiscNum);
	ATTRIBUTE_ACCESSORS(UBaseAttributeSet, SkillCD);

	ATTRIBUTE_ACCESSORS(UBaseAttributeSet, MAX_PassiveGA);
	ATTRIBUTE_ACCESSORS(UBaseAttributeSet, MAX_OffensiveGA);

	ATTRIBUTE_ACCESSORS(UBaseAttributeSet, Health);
	ATTRIBUTE_ACCESSORS(UBaseAttributeSet, MaxHealth);

	ATTRIBUTE_ACCESSORS(UBaseAttributeSet, OutRoundLifeTime);

	ATTRIBUTE_ACCESSORS(UBaseAttributeSet, MoveSpeed);


	ATTRIBUTE_ACCESSORS(UBaseAttributeSet, Dash);
	ATTRIBUTE_ACCESSORS(UBaseAttributeSet, DashCD);
	ATTRIBUTE_ACCESSORS(UBaseAttributeSet, DashDist);


	ATTRIBUTE_ACCESSORS(UBaseAttributeSet, Dodge);
	ATTRIBUTE_ACCESSORS(UBaseAttributeSet, Resilience);
	ATTRIBUTE_ACCESSORS(UBaseAttributeSet, Resist);
    ATTRIBUTE_ACCESSORS(UBaseAttributeSet, Shield);


	//////////////////////////////////////////////// Ability Attribute ////////////////////////////////////////////////
	ATTRIBUTE_ACCESSORS(UBaseAttributeSet, ActDamage);
	ATTRIBUTE_ACCESSORS(UBaseAttributeSet, ActRange);
	ATTRIBUTE_ACCESSORS(UBaseAttributeSet, ActResilience);
	ATTRIBUTE_ACCESSORS(UBaseAttributeSet, ActDmgReduce);
	ATTRIBUTE_ACCESSORS(UBaseAttributeSet, ActRotateSpeed);
	ATTRIBUTE_ACCESSORS(UBaseAttributeSet, JumpFrameTime);
	ATTRIBUTE_ACCESSORS(UBaseAttributeSet, FreezeFrameTime);


	//////////////////////////////////////////////// Weapon Attribute ////////////////////////////////////////////////
	ATTRIBUTE_ACCESSORS(UBaseAttributeSet, WeaponAtk);
	ATTRIBUTE_ACCESSORS(UBaseAttributeSet, WeaponAtkSpeed);
	ATTRIBUTE_ACCESSORS(UBaseAttributeSet, WeaponRange);
	ATTRIBUTE_ACCESSORS(UBaseAttributeSet, CriticalRate);
	ATTRIBUTE_ACCESSORS(UBaseAttributeSet, CriticalDamage);



	//////////////////////////////////////////////// Damager Attribute ////////////////////////////////////////////////
	ATTRIBUTE_ACCESSORS(UBaseAttributeSet, Damage);




protected:
	void AdjustAttributeForMaxChange(FGameplayAttributeData& AffectedAttribute, const FGameplayAttributeData& MaxAttribute, float NewMaxValue, const FGameplayAttribute& AffectedAttributeProperty);

public:



    ////////////////////////////////////////////////// Player Attribute ////////////////////////////////////////////////
    UPROPERTY(BlueprintReadWrite, Category = "Attributes|Player")
    FGameplayAttributeData Attack;
    UPROPERTY(BlueprintReadWrite, Category = "Attributes|Player")
    FGameplayAttributeData AttackPower;

    UPROPERTY(BlueprintReadWrite, Category = "Attributes|Player")
    FGameplayAttributeData MiscNum;
    UPROPERTY(BlueprintReadWrite, Category = "Attributes|Player")
    FGameplayAttributeData SkillCD;

    UPROPERTY(BlueprintReadWrite, Category = "Attributes|Player")
    FGameplayAttributeData MAX_PassiveGA;
    UPROPERTY(BlueprintReadWrite, Category = "Attributes|Player")
    FGameplayAttributeData MAX_OffensiveGA;

    UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Health, Category = "Health")
    FGameplayAttributeData Health;
    UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxHealth, Category = "Health")
    FGameplayAttributeData MaxHealth;

    UPROPERTY(BlueprintReadWrite, Category = "Attributes|Player")
    FGameplayAttributeData OutRoundLifeTime;

    UPROPERTY(BlueprintReadWrite, Category = "Attributes|Player")
    FGameplayAttributeData MoveSpeed;

    UPROPERTY(BlueprintReadWrite, Category = "Attributes|Player")
    FGameplayAttributeData Dash;
    UPROPERTY(BlueprintReadWrite, Category = "Attributes|Player")
    FGameplayAttributeData DashCD;
    UPROPERTY(BlueprintReadWrite, Category = "Attributes|Player")
    FGameplayAttributeData DashDist;

    UPROPERTY(BlueprintReadWrite, Category = "Attributes|Player")
    FGameplayAttributeData Dodge;
    UPROPERTY(BlueprintReadWrite, Category = "Attributes|Player")
    FGameplayAttributeData Resilience;
    UPROPERTY(BlueprintReadWrite, Category = "Attributes|Player")
    FGameplayAttributeData Resist;
    UPROPERTY(BlueprintReadWrite, Category = "Attributes|Player")
    FGameplayAttributeData Shield;

    //////////////////////////////////////////////// Ability Attribute ////////////////////////////////////////////////
    UPROPERTY(BlueprintReadWrite, Category = "Attributes|Ability")
    FGameplayAttributeData ActDamage;
    UPROPERTY(BlueprintReadWrite, Category = "Attributes|Ability")
    FGameplayAttributeData ActRange;
    UPROPERTY(BlueprintReadWrite, Category = "Attributes|Ability")
    FGameplayAttributeData ActResilience;
    UPROPERTY(BlueprintReadWrite, Category = "Attributes|Ability")
    FGameplayAttributeData ActDmgReduce;
    UPROPERTY(BlueprintReadWrite, Category = "Attributes|Ability")
    FGameplayAttributeData ActRotateSpeed;
    UPROPERTY(BlueprintReadWrite, Category = "Attributes|Ability")
    FGameplayAttributeData JumpFrameTime;
    UPROPERTY(BlueprintReadWrite, Category = "Attributes|Ability")
    FGameplayAttributeData FreezeFrameTime;

    //////////////////////////////////////////////// Weapon Attribute ////////////////////////////////////////////////
    UPROPERTY(BlueprintReadWrite, Category = "Attributes|Weapon")
    FGameplayAttributeData WeaponAtk;
    UPROPERTY(BlueprintReadWrite, Category = "Attributes|Weapon")
    FGameplayAttributeData WeaponAtkSpeed;
    UPROPERTY(BlueprintReadWrite, Category = "Attributes|Weapon")
    FGameplayAttributeData WeaponRange;
    UPROPERTY(BlueprintReadWrite, Category = "Attributes|Weapon")
    FGameplayAttributeData CriticalRate;  // Possibly a typo in "CriticalRate"?
    UPROPERTY(BlueprintReadWrite, Category = "Attributes|Weapon")
    FGameplayAttributeData CriticalDamage;



    //////////////////////////////////////////////// Damager Attribute ////////////////////////////////////////////////
    UPROPERTY(BlueprintReadWrite, Category = "Damage")
    FGameplayAttributeData Damage;



    float INIT_ActDamage = 20;
    float INIT_ActRange = 400;
    float INIT_ActResilience = 20;
    float INIT_ActDmgReduce = 0;
    float INIT_ActRotateSpeed = 360;
    float INIT_JumpFrameTime = 0.15;
    float INIT_FreezeFrameTime = 0.15;


	UFUNCTION()
	void OnRep_Health(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_MaxHealth(const FGameplayAttributeData& OldValue);


	UFUNCTION()
	void InitAttribute();

    UFUNCTION()
    void ResetAbilityAttribute();

    UFUNCTION()
    void InitCharacterData(const FYogCharacterData& data);


    //const FMovementData& moveData = CharacterData->GetMovementData();
    //const FYogCharacterData& characterData = CharacterData->GetCharacterData();


};
