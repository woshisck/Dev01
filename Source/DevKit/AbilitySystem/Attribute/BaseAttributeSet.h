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
    UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_Attack, Category = "Attributes|Player")
    FGameplayAttributeData Attack;
    ATTRIBUTE_ACCESSORS(UBaseAttributeSet, Attack);

    UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_AttackPower, Category = "Attributes|Player")
    FGameplayAttributeData AttackPower;
    ATTRIBUTE_ACCESSORS(UBaseAttributeSet, AttackPower);

    UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Health, Category = "Health")
    FGameplayAttributeData Health;
    ATTRIBUTE_ACCESSORS(UBaseAttributeSet, Health);

    UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxHealth, Category = "Health")
    FGameplayAttributeData MaxHealth;
    ATTRIBUTE_ACCESSORS(UBaseAttributeSet, MaxHealth);

    UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_Shield, Category = "Attributes|Player")
    FGameplayAttributeData Shield;
    ATTRIBUTE_ACCESSORS(UBaseAttributeSet, Shield);

    UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_AttackSpeed, Category = "Attributes|Player")
    FGameplayAttributeData AttackSpeed;
    ATTRIBUTE_ACCESSORS(UBaseAttributeSet, AttackSpeed);

    UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_AttackRange, Category = "Attributes|Player")
    FGameplayAttributeData AttackRange;
    ATTRIBUTE_ACCESSORS(UBaseAttributeSet, AttackRange);

    UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_Sanity, Category = "Attributes|Player")
    FGameplayAttributeData Sanity;
    ATTRIBUTE_ACCESSORS(UBaseAttributeSet, Sanity);

    UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_Sanity, Category = "Attributes|Player")
    FGameplayAttributeData MoveSpeed;
    ATTRIBUTE_ACCESSORS(UBaseAttributeSet, MoveSpeed);


    UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_Sanity, Category = "Attributes|Player")
    FGameplayAttributeData Dodge;
    ATTRIBUTE_ACCESSORS(UBaseAttributeSet, Dodge);


    UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_Sanity, Category = "Attributes|Player")
    FGameplayAttributeData Resilience;
    ATTRIBUTE_ACCESSORS(UBaseAttributeSet, Resilience);
    

    UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_Sanity, Category = "Attributes|Player")
    FGameplayAttributeData Resist;
    ATTRIBUTE_ACCESSORS(UBaseAttributeSet, Resist);

    UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_Sanity, Category = "Attributes|Player")
    FGameplayAttributeData DmgTaken;
    ATTRIBUTE_ACCESSORS(UBaseAttributeSet, DmgTaken);

    UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_Sanity, Category = "Attributes|Player")
    FGameplayAttributeData Crit_Rate;
    ATTRIBUTE_ACCESSORS(UBaseAttributeSet, Crit_Rate);


    UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_Sanity, Category = "Attributes|Player")
    FGameplayAttributeData Crit_Damage;
    ATTRIBUTE_ACCESSORS(UBaseAttributeSet, Crit_Damage);



    UFUNCTION()
    void Init(UCharacterData* data);


	UFUNCTION()
	void OnRep_Health(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_MaxHealth(const FGameplayAttributeData& OldValue);


    UFUNCTION()
    void OnRep_Attack(const FGameplayAttributeData& OldValue);


    UFUNCTION()
    void OnRep_AttackPower(const FGameplayAttributeData& OldValue);


    UFUNCTION()
    void OnRep_Shield(const FGameplayAttributeData& OldValue);


    UFUNCTION()
    void OnRep_AttackSpeed(const FGameplayAttributeData& OldValue);


    UFUNCTION()
    void OnRep_Sanity(const FGameplayAttributeData& OldValue);

    UFUNCTION()
    void OnRep_MoveSpeed(const FGameplayAttributeData& OldValue);

    UFUNCTION()
    void OnRep_Dodge(const FGameplayAttributeData& OldValue);

    UFUNCTION()
    void OnRep_Resilience(const FGameplayAttributeData& OldValue);

    UFUNCTION()
    void OnRep_Resist(const FGameplayAttributeData& OldValue);

    UFUNCTION()
    void OnRep_DmgTaken(const FGameplayAttributeData& OldValue);

    UFUNCTION()
    void OnRep_Crit_Rate(const FGameplayAttributeData& OldValue);

    UFUNCTION()
    void OnRep_Crit_Damage(const FGameplayAttributeData& OldValue);

    UFUNCTION()
    void OnRep_AttackRange(const FGameplayAttributeData& OldValue);

    
    //const FMovementData& moveData = CharacterData->GetMovementData();
    //const FYogBaseAttributeData& characterData = CharacterData->GetBaseAttributeData();


};
