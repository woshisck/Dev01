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
class UInventoryManagerComponent;
class UYogGameplayAbility;

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


	AYogBaseCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintCallable, Category = "Character")
	UYogAbilitySystemComponent* GetYogAbilitySystemComponent() const;
	
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;


	UFUNCTION(BlueprintCallable)
	virtual int32 GetCharacterLevel() const;
	//int32 ARPGCharacterBase::GetCharacterLevel() const
	//{
	//	return CharacterLevel;
	//}



	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsDead;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bCanMove;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UBaseAttributeSet> AttributeSet;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "GASDocumentation|Animation")
	UAnimMontage* DeathMontage;

	//Passive effect
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Abilities)
	TArray<TSubclassOf<UGameplayEffect>> PassiveGameplayEffects;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "AblitySystemComp")
	TObjectPtr<UYogAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTagContainer GPTagsContainer;


	UFUNCTION(BlueprintCallable, Category = "Character|Attributes")
	float GetHealth() const;


	UFUNCTION(BlueprintCallable, Category = "Character|Attributes")
	float GetMaxHealth() const;

	UFUNCTION(BlueprintCallable, Category = "Character|Abilities")
	void GrantGameplayAbility(TSubclassOf<UYogGameplayAbility> AbilityToGrant, int32 AbilityLevel);



	UPROPERTY(BlueprintReadOnly)
	int32 bWeaponEquiped = 0;

	UFUNCTION(BlueprintCallable, Category = "Character|Debug")
	void PrintAllGameplayTags(const FGameplayTagContainer& TagContainer);

protected:

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	TObjectPtr<UInventoryManagerComponent> InventoryManagerComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character|Abilities")
	TArray<TSubclassOf<UYogGameplayAbility>> GameplayAbilities;



	UPROPERTY(EditAnywhere, Category = "Character|Abilities")
	int32 CharacterLevel;

	UPROPERTY()
	int32 bAbilitiesInitialized;

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
