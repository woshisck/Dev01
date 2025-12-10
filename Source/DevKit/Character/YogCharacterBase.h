// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ModularCharacter.h"
#include "AbilitySystem/Attribute/BaseAttributeSet.h"
#include "AbilitySystem/Attribute/DamageAttributeSet.h"


#include "AbilitySystemInterface.h"
#include "../Data/CharacterData.h"
#include "DevKit/Component/AttributeStatComponent.h"
#include "YogCharacterBase.generated.h"



class AYogPlayerControllerBase;


UENUM(BlueprintType)
enum class EYogCharacterState : uint8
{
	Idle					UMETA(DisplayName = "Idle"),
	Move					UMETA(DisplayName = "Move"),
	OnAction				UMETA(DisplayName = "OnAction"),
	OnHurt					UMETA(DisplayName = "OnHurt"),
	Stun					UMETA(DisplayName = "Stun"),
	None					UMETA(DisplayName = "None")
};



class UItemInstance;
class UYogAbilitySystemComponent;
class UHitBoxBufferComponent;
class UYogGameplayAbility;
class AItemSpawner;
class AWeaponInstance;


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCharacterDiedDelegate, AYogCharacterBase*, Character);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCharacterHealthUpdateDelegate, const float, HealthPercent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCharacterMoveableDelegate, const bool, Moveable);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCharacterVelocityDelegate, const FVector, Velocity);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FCharacterStateDelegate, EYogCharacterState, StateBefore, EYogCharacterState, StateAfter);



/**
 * 
 */
UCLASS()
class DEVKIT_API AYogCharacterBase : public AModularCharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()


public:
	void BeginPlay() override;
	void Tick(float DeltaSeconds) override;
	void PostInitializeComponents() override;

	AYogCharacterBase(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());



	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attribute", meta = (EditCondition = "bAllowEditInChildClass"))
	TObjectPtr<UCharacterData> CharacterData;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame)
	TObjectPtr<UAttributeStatComponent> AttributeStatsComponent;

	////////////////////////////////////////// Attribute Set //////////////////////////////////////////

	UPROPERTY(SaveGame, BlueprintReadWrite)
	TObjectPtr<UBaseAttributeSet> BaseAttributeSet;

	UPROPERTY(SaveGame, BlueprintReadWrite)
	TObjectPtr<UDamageAttributeSet> DamageAttributeSet;

	///////////////////////////////////////////　Passive effect　///////////////////////////////////////////

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = Abilities)
	TArray<TSubclassOf<UGameplayEffect>> PassiveGameplayEffects;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, SaveGame, Category = "AblitySystemComp")
	TObjectPtr<UYogAbilitySystemComponent> AbilitySystemComponent;

	////////////////////////////////////////// Get/Set Func　//////////////////////////////////////////

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character|Animation")
	TObjectPtr<UAnimMontage> DeathMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character|Animation")
	TObjectPtr<UAnimMontage> HurtMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character|Ability")
	TObjectPtr<UAbilityData> AbilityData;


	//UPROPERTY(VisibleAnywhere, SaveGame, BlueprintReadWrite, Category = "Weapon")
	//TObjectPtr<AWeaponInstance> Weapon;

	//DELEGATE DEFINE
	UPROPERTY(BlueprintAssignable, SaveGame, Category = "Character|Attributes")
	FCharacterDiedDelegate OnCharacterDied;

	UPROPERTY(BlueprintAssignable, Category = "Character|Attributes")
	FCharacterHealthUpdateDelegate OnCharacterHealthUpdate;


	UPROPERTY(BlueprintAssignable, Category = "Character|Attributes")
	FCharacterMoveableDelegate OnCharacterCanMoveUpdate;

	UPROPERTY(BlueprintAssignable, Category = "Character|Movement")
	FCharacterVelocityDelegate OnCharacterVelocityUpdate;


	UPROPERTY(BlueprintAssignable, Category = "Character|State")
	FCharacterStateDelegate OnCharacterStateUpdate;


	UFUNCTION(BlueprintCallable)
	EYogCharacterState GetCurrentState();

	UFUNCTION(BlueprintCallable, Category = "Character|Attributes")
	void GetActiveAbilitiesWithTags(FGameplayTagContainer AbilityTags, TArray<UYogGameplayAbility*>& ActiveAbilities);


	UFUNCTION(BlueprintCallable, Category = "Character|Attributes")
	float GetHealth() const;


	UFUNCTION(BlueprintCallable, Category = "Character|Attributes")
	float GetMaxHealth() const;

	UFUNCTION(BlueprintCallable, Category = "Character|Attributes")
	float GetAtkDist() const;

	UFUNCTION(BlueprintCallable, Category = "Character|Abilities")
	void GrantGameplayAbility(TSubclassOf<UYogGameplayAbility> AbilityToGrant, int32 AbilityLevel);


	UFUNCTION(BlueprintCallable, Category = "Character|Debug")
	void PrintAllGameplayTags(const FGameplayTagContainer& TagContainer);



	///** Map of gameplay tags to gameplay effect containers */
	//UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Character|Buff")
	//TMap<FGameplayTag, FYogGameplayEffectContainer> BufferMap;


	UFUNCTION(BlueprintCallable, Category = "Character")
	UYogAbilitySystemComponent* GetASC() const;

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	UFUNCTION(BlueprintCallable)
	FVector GetGroundSlope(float length);

	UFUNCTION(BlueprintCallable)
	void UpdateCharacterMovement(const bool IsMovable);

	UFUNCTION(BlueprintCallable, Category = "Character|Movement")
	void DisableMovement();

	UFUNCTION(BlueprintCallable, Category = "Character|Movement")
	void EnableMovement();

	UFUNCTION(BlueprintCallable, Category = "Character|State")
	void UpdateCharacterState(EYogCharacterState newState, FVector movementInput);


public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsDead;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bMovable;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	TObjectPtr<UHitBoxBufferComponent> HitboxbuffComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character|Abilities")
	TArray<TSubclassOf<UYogGameplayAbility>> GameplayAbilities;


	FDelegateHandle HealthChangedDelegateHandle;
	FDelegateHandle MaxHealthChangedDelegateHandle;

	//Attribute change delegate
	virtual void HealthChanged(const FOnAttributeChangeData& Data);
	virtual void MaxHealthChanged(const FOnAttributeChangeData& Data);



	//UFUNCTION(BlueprintCallable, Category = "Feature")
	virtual bool IsAlive() const;

	//UFUNCTION(BlueprintCallable, Category = "Feature")
	virtual void FinishDying();

	//UFUNCTION(BlueprintCallable, Category = "Feature")
	
	virtual void Die();
	// Friended to allow access to handle functions above
	friend UBaseAttributeSet;
	friend UDamageAttributeSet;
	//friend UAdditionAttributeSet;

private:
	UPROPERTY()
	EYogCharacterState CurrentState;

	UPROPERTY()
	EYogCharacterState PreviousState;
};
