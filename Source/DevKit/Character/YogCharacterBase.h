// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ModularCharacter.h"
#include <DevKit/AbilitySystem/Attribute/BaseAttributeSet.h>
#include "AbilitySystemInterface.h"

#include "YogCharacterBase.generated.h"



UENUM()
enum class EYogCharacterState : uint8
{
	Move					UMETA(DisplayName = "Move"),
	Idle					UMETA(DisplayName = "Idle"),
	AbilityCast				UMETA(DisplayName = "AbilityCast"),
	Hit						UMETA(DisplayName = "Hit")

};



class UItemInstance;
class UYogAbilitySystemComponent;
class UInventoryManagerComponent;
class UYogGameplayAbility;
class AItemSpawner;



DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCharacterDiedDelegate, AYogCharacterBase*, Character);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCharacterHealthUpdateDelegate, const float, HealthPercent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCharacterMoveableDelegate, const bool, Moveable);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCharacterVelocityDelegate, const FVector, Velocity);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FCharacterStateDelegate, EYogCharacterState, State, const FVector, MovementInput);

USTRUCT(BlueprintType)
struct FCharacterMovementData : public FTableRowBase
{
	GENERATED_BODY()

public:
	FCharacterMovementData()
		: MaxWalkSpeed(600.0f), GroundFriction(8.0f), BreakingDeceleration(2048.0f), MaxAcceleration(2048.0f), RotationRate(FRotator(0,0,360))
	{
	}
	//UPROPERTY(EditAnywhere, BlueprintReadWrite)
	//FString Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MaxWalkSpeed;
	 
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float GroundFriction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float BreakingDeceleration;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MaxAcceleration;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FRotator RotationRate;
};


class AYogPlayerControllerBase;

/**
 * 
 */
UCLASS()
class DEVKIT_API AYogCharacterBase : public AModularCharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()


public:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;


	AYogCharacterBase(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintCallable, Category = "Character")
	UYogAbilitySystemComponent* GetASC() const;
	
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;


	UFUNCTION(BlueprintCallable)
	void UpdateCharacterMovement(const bool IsMovable);

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	EYogCharacterState CurrentState;


	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bInstantRotate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UBaseAttributeSet> AttributeSet;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "GASDocumentation|Animation")
	UAnimMontage* DeathMontage;

	///////////////////////////////////////////　Passive effect　///////////////////////////////////////////

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Abilities)
	TArray<TSubclassOf<UGameplayEffect>> PassiveGameplayEffects;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "AblitySystemComp")
	TObjectPtr<UYogAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTagContainer GPTagsContainer;

	////////////////////////////////////////////// Attribute //////////////////////////////////////////////

	UFUNCTION(BlueprintCallable, Category = "Character|Attributes")
	void GetActiveAbilitiesWithTags(FGameplayTagContainer AbilityTags, TArray<UYogGameplayAbility*>& ActiveAbilities);


	UFUNCTION(BlueprintCallable, Category = "Character|Attributes")
	float GetHealth() const;


	UFUNCTION(BlueprintCallable, Category = "Character|Attributes")
	float GetMaxHealth() const;

	///////////////////////////////////////////////////////////////////////////////////////////////////////


	UFUNCTION(BlueprintCallable, Category = "Character|Abilities")
	void GrantGameplayAbility(TSubclassOf<UYogGameplayAbility> AbilityToGrant, int32 AbilityLevel);


	UPROPERTY(BlueprintReadOnly)
	int32 bWeaponEquiped = 0;

	UFUNCTION(BlueprintCallable, Category = "Character|Debug")
	void PrintAllGameplayTags(const FGameplayTagContainer& TagContainer);

	//DELEGATE DEFINE
	UPROPERTY(BlueprintAssignable, Category = "Character|Attributes")
	FCharacterDiedDelegate OnCharacterDied;

	UPROPERTY(BlueprintAssignable, Category = "Character|Attributes")
	FCharacterHealthUpdateDelegate OnCharacterHealthUpdate;

	UPROPERTY(BlueprintAssignable, Category = "Character|Attributes")
	FCharacterMoveableDelegate OnCharacterCanMoveUpdate;

	UPROPERTY(BlueprintAssignable, Category = "Character|Movement")
	FCharacterVelocityDelegate OnCharacterVelocityUpdate;


	UPROPERTY(BlueprintAssignable, Category = "Character|Movement")
	FCharacterStateDelegate OnCharacterStateUpdate;

	UFUNCTION(BlueprintCallable, Category = "Character|Movement")
	void DisableMovement();

	UFUNCTION(BlueprintCallable, Category = "Character|Movement")
	void EnableMovement();

	UFUNCTION(BlueprintCallable, Category = "Character|State")
	void SetCharacterState(EYogCharacterState newState, FVector movementInput);





	///** Map of gameplay tags to gameplay effect containers */
	//UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Character|Buff")
	//TMap<FGameplayTag, FYogGameplayEffectContainer> BufferMap;


public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsDead;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bMovable;


	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	TObjectPtr<UInventoryManagerComponent> InventoryManagerComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character|Abilities")
	TArray<TSubclassOf<UYogGameplayAbility>> GameplayAbilities;



	UPROPERTY()
	int32 bAbilitiesInitialized;

	FGameplayTag DeadTag;

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
