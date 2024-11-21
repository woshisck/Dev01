// Fill out your copyright notice in the Description page of Project Settings.

#pragma once



#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "BaseAttributeSet.generated.h"


class AActor;
class ULyraAbilitySystemComponent;
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
