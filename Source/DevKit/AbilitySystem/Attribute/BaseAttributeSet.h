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



/**
 * 
 */
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



	ATTRIBUTE_ACCESSORS(UBaseAttributeSet, Health);
	ATTRIBUTE_ACCESSORS(UBaseAttributeSet, MaxHealth);

	ATTRIBUTE_ACCESSORS(UBaseAttributeSet, BaseDMG);
	ATTRIBUTE_ACCESSORS(UBaseAttributeSet, WeaponDMG);

	ATTRIBUTE_ACCESSORS(UBaseAttributeSet, BuffAmplify);
	ATTRIBUTE_ACCESSORS(UBaseAttributeSet, DMGAbsorb);

	//ATTRIBUTE_ACCESSORS(UBaseAttributeSet, BuffingATK);


	//ATTRIBUTE_ACCESSORS(UBaseAttributeSet, OwnerSpeed);


	//ATTRIBUTE_ACCESSORS(UBaseAttributeSet, DMGCorrect);
	
	//ATTRIBUTE_ACCESSORS(UBaseAttributeSet, HitRate);
	//ATTRIBUTE_ACCESSORS(UBaseAttributeSet, Evade);

	ATTRIBUTE_ACCESSORS(UBaseAttributeSet, Damage);

protected:
	void AdjustAttributeForMaxChange(FGameplayAttributeData& AffectedAttribute, const FGameplayAttributeData& MaxAttribute, float NewMaxValue, const FGameplayAttribute& AffectedAttributeProperty);

public:

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Health, Category = "Health")
	FGameplayAttributeData Health;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxHealth, Category = "Health")
	FGameplayAttributeData MaxHealth;



	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_BaseDMG, Category = "Combat")
	FGameplayAttributeData BaseDMG;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_WeaponDMG, Category = "Combat")
	FGameplayAttributeData WeaponDMG;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_BuffAmplify, Category = "Combat")
	FGameplayAttributeData BuffAmplify;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_DMGAbsorb, Category = "Combat")
	FGameplayAttributeData DMGAbsorb;

	UPROPERTY(BlueprintReadWrite, Category = "Damage")
	FGameplayAttributeData Damage;


	UFUNCTION()
	void OnRep_Health(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_MaxHealth(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_BaseDMG(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_WeaponDMG(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_BuffAmplify(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_DMGAbsorb(const FGameplayAttributeData& OldValue);

};
