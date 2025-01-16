// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ModularCharacter.h"
#include <DevKit/AbilitySystem/Attribute/BaseAttributeSet.h>
#include "AbilitySystemInterface.h"
#include "YogBaseCharacter.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCharacterDiedDelegate, AYogBaseCharacter*, Character);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCharacterHealthUpdateDelegate, const float, HealthPercent);


class UItemInstance;
class UYogAbilitySystemComponent;

/**
 * 
 */
UCLASS()
class DEVKIT_API AYogBaseCharacter : public AModularCharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()



public:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	//Interface  IAbilitySystemInterface
	class UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	AYogBaseCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());


	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsDead;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bCanMove;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UBaseAttributeSet> AttributeSet;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "GASDocumentation|Animation")
	UAnimMontage* DeathMontage;

	//SKill
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Abilities)
	TArray<TSubclassOf<UGameplayEffect>> PassiveGameplayEffects;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "AblitySystemComp")
	TObjectPtr<UYogAbilitySystemComponent> AbilitySystemComponent;


	UFUNCTION(BlueprintCallable, Category = "Character|Attributes")
	float GetHealth() const;


	UFUNCTION(BlueprintCallable, Category = "Character|Attributes")
	float GetMaxHealth() const;

	/** Apply the startup gameplay abilities and effects */
	void AddStartupGameplayAbilities();


protected:



	FGameplayTag DeadTag;

	UPROPERTY(BlueprintAssignable, Category = "Character|Attributes")
	FCharacterDiedDelegate OnCharacterDied;

	UPROPERTY(BlueprintAssignable, Category = "Character|Attributes")
	FCharacterHealthUpdateDelegate OnCharacterHealthUpdate;

	FDelegateHandle HealthChangedDelegateHandle;
	FDelegateHandle MaxHealthChangedDelegateHandle;
	FDelegateHandle BaseDMGChangedDelegateHandle;
	FDelegateHandle WeaponDMGChangedDelegateHandle;
	FDelegateHandle BuffAmplifyChangedDelegateHandle;


	//Attribute change delegate
	virtual void HealthChanged(const FOnAttributeChangeData& Data);
	virtual void MaxHealthChanged(const FOnAttributeChangeData& Data);
	virtual void BaseDMGChanged(const FOnAttributeChangeData& Data);
	virtual void WeaponDMGChanged(const FOnAttributeChangeData& Data);
	virtual void BuffAmplifyChanged(const FOnAttributeChangeData& Data);



	UFUNCTION(BlueprintCallable, Category = "Feature")
	virtual bool IsAlive() const;

	UFUNCTION(BlueprintCallable, Category = "Feature")
	virtual void FinishDying();

	UFUNCTION(BlueprintCallable, Category = "Feature")
	virtual void Die();
	// Friended to allow access to handle functions above
	friend UBaseAttributeSet;
};
