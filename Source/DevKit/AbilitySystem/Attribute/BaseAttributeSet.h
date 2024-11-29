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

DECLARE_MULTICAST_DELEGATE_SixParams(FLyraAttributeEvent, AActor* /*EffectInstigator*/, AActor* /*EffectCauser*/, const FGameplayEffectSpec* /*EffectSpec*/, float /*EffectMagnitude*/, float /*OldValue*/, float /*NewValue*/);


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
	//ATTRIBUTE_ACCESSORS(UBaseAttributeSet, BuffingATK);


	//ATTRIBUTE_ACCESSORS(UBaseAttributeSet, OwnerSpeed);


	//ATTRIBUTE_ACCESSORS(UBaseAttributeSet, DMGCorrect);
	//ATTRIBUTE_ACCESSORS(UBaseAttributeSet, DMGAbsorb);
	//ATTRIBUTE_ACCESSORS(UBaseAttributeSet, HitRate);
	//ATTRIBUTE_ACCESSORS(UBaseAttributeSet, Evade);

	ATTRIBUTE_ACCESSORS(UBaseAttributeSet, DamageResult);

protected:
	void AdjustAttributeForMaxChange(FGameplayAttributeData& AffectedAttribute, const FGameplayAttributeData& MaxAttribute, float NewMaxValue, const FGameplayAttribute& AffectedAttributeProperty);

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, ReplicatedUsing = OnRep_Health, Category = "Health")
	FGameplayAttributeData Health;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, ReplicatedUsing = OnRep_MaxHealth, Category = "Health")
	FGameplayAttributeData MaxHealth;



	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_BaseDMG, Category = "Combat")
	FGameplayAttributeData BaseDMG;

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_WeaponDMG, Category = "Combat")
	FGameplayAttributeData WeaponDMG;

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_BuffAmplify, Category = "Combat")
	FGameplayAttributeData BuffAmplify;

	//UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_BuffingATK, Category = "Combat")
	//FGameplayAttributeData BuffingATK;

	//UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_OwnerSpeed, Category = "Combat")
	//FGameplayAttributeData OwnerSpeed;

	//UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_DMGCorrect, Category = "Combat")
	//FGameplayAttributeData DMGCorrect;

	//UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_DMGAbsorb, Category = "Combat")
	//FGameplayAttributeData DMGAbsorb;

	//UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_HitRate, Category = "Combat")
	//FGameplayAttributeData HitRate;

	//UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_Evade, Category = "Combat")
	//FGameplayAttributeData Evade;

	UPROPERTY(BlueprintReadWrite, Category = "DamageResult")
	FGameplayAttributeData DamageResult;



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
	//UFUNCTION()
	//void OnRep_BuffingATK(const FGameplayAttributeData& OldValue);
	//UFUNCTION()
	//void OnRep_OwnerSpeed(const FGameplayAttributeData& OldValue);
	//UFUNCTION()
	//void OnRep_DMGCorrect(const FGameplayAttributeData& OldValue);
	//UFUNCTION()
	//void OnRep_DMGAbsorb(const FGameplayAttributeData& OldValue);
	//UFUNCTION()
	//void OnRep_HitRate(const FGameplayAttributeData& OldValue);
	//UFUNCTION()
	//void OnRep_Evade(const FGameplayAttributeData& OldValue);

	//UPROPERTY(EditAnywhere, BlueprintReadWrite)
	//FGameplayAttributeData Health = 100.0f;
	//ATTRIBUTE_ACCESSORS(UBaseAttributeSet, Health);

	//UPROPERTY(EditAnywhere, BlueprintReadWrite)
	//FGameplayAttributeData MaxHealth = 100.0f;
	//ATTRIBUTE_ACCESSORS(UBaseAttributeSet, MaxHealth)

	//UPROPERTY(EditAnywhere, BlueprintReadWrite)
	//FGameplayAttributeData  WeaponDMG = 100.0f;
	//ATTRIBUTE_ACCESSORS(UBaseAttributeSet, WeaponDMG)

	//UPROPERTY(EditAnywhere, BlueprintReadWrite)
	//FGameplayAttributeData OwnerDMG = 100.0f;
	//ATTRIBUTE_ACCESSORS(UBaseAttributeSet, OwnerDMG)

	//UPROPERTY(EditAnywhere, BlueprintReadWrite)
	//FGameplayAttributeData DMGAmplify = 1.0f;
	//ATTRIBUTE_ACCESSORS(UBaseAttributeSet, DMGAmplify)

	//UPROPERTY(EditAnywhere, BlueprintReadWrite)
	//FGameplayAttributeData OwnerSpeed = 100.0f;
	//ATTRIBUTE_ACCESSORS(UBaseAttributeSet, OwnerSpeed)

	//UPROPERTY(EditAnywhere, BlueprintReadWrite)
	//FGameplayAttributeData DMGCorrect = 1.0f;
	//ATTRIBUTE_ACCESSORS(UBaseAttributeSet, DMGCorrect)

	//UPROPERTY(EditAnywhere, BlueprintReadWrite)
	//FGameplayAttributeData DMGAbsorb = 1.0f;
	//ATTRIBUTE_ACCESSORS(UBaseAttributeSet, DMGAbsorb)

	//UPROPERTY(EditAnywhere, BlueprintReadWrite)
	//FGameplayAttributeData HitRate = 1.0f;
	//ATTRIBUTE_ACCESSORS(UBaseAttributeSet, HitRate)

	//UPROPERTY(EditAnywhere, BlueprintReadWrite)
	//FGameplayAttributeData Evade = 0.0f;
	//ATTRIBUTE_ACCESSORS(UBaseAttributeSet, Evade)


	//UFUNCTION()
	//void OnRep_Health(const FGameplayAttributeData& OldHealth) const;

	//UFUNCTION()
	//void OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth) const;

};
