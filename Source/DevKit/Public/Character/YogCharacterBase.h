// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ModularCharacter.h"
#include "AbilitySystem/Attribute/BaseAttributeSet.h"
#include "AbilitySystem/Attribute/DamageAttributeSet.h"
#include "AbilitySystem/Attribute/RuneAttributeSet.h"


#include "AbilitySystemInterface.h"
#include "Data/CharacterData.h"
#include "Component/AttributeStatComponent.h"
#include "YogCharacterBase.generated.h"



class AYogPlayerControllerBase;
class UGASTemplate;
class UGameEffectComponent;
class UCharacterDataComponent;
class UBufferComponent;

UENUM(BlueprintType)
enum class EYogCharacterState : uint8
{
	Idle					UMETA(DisplayName = "Idle"),
	Move					UMETA(DisplayName = "Move"),
	Action				UMETA(DisplayName = "Action"),
	GetHit					UMETA(DisplayName = "GetHit"),
	Stun					UMETA(DisplayName = "Stun"),
	Dead					UMETA(DisplayName = "Dead")
};

UENUM(BlueprintType)
enum class EWeaponState : uint8
{
	Unequipped		UMETA(DisplayName = "Unequipped"), 
	Equipping		UMETA(DisplayName = "Equipping"), 
	Equipped		UMETA(DisplayName = "Equipped"), 
	Firing			UMETA(DisplayName = "Firing"), 
	Reloading		UMETA(DisplayName = "Reloading")
};




class UItemInstance;
class UYogAbilitySystemComponent;
class UHitBoxBufferComponent;
class UYogGameplayAbility;
class AItemSpawner;
class AWeaponInstance;
class UCharacterData;
class UPropInteractComponnet;
class UWidgetComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCharacterDiedDelegate, AYogCharacterBase*, Character);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCharacterHealthUpdateDelegate, const float, HealthPercent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCharacterMoveableDelegate, const bool, Moveable);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCharacterVelocityDelegate, const FVector, Velocity);





DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FCharacterStateDelegate, EYogCharacterState, StateBefore, EYogCharacterState, StateAfter);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FWeaponStateDelegate, EWeaponState, StateBefore, EWeaponState, StateAfter);


/**
 * 
 */
UCLASS()
class DEVKIT_API AYogCharacterBase : public AModularCharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()


public:
	AYogCharacterBase(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;


	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void PostInitializeComponents() override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void UnPossessed() override;
	virtual void OnRep_PlayerState() override;
	virtual void OnRep_Controller() override;

	//---------------------------------------
	//	Components
	//---------------------------------------

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AblitySystemComp")
	TObjectPtr<UYogAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UCharacterDataComponent> CharacterDataComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UPropInteractComponnet> PropInteractComponent;

	UPROPERTY()
	TObjectPtr<UBaseAttributeSet> BaseAttributeSet;

	UPROPERTY()
	TObjectPtr<UDamageAttributeSet> DamageAttributeSet;

	UPROPERTY()
	TObjectPtr<URuneAttributeSet> RuneAttributeSet;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UWidgetComponent> WidgetComponent;



	UAttributeStatComponent* GetAttributeStatsComponent() const;
	UGameEffectComponent* GetGameEffectComponent() const;
	UBufferComponent* GetInputBufferComponent() const;
	UCharacterDataComponent* GetCharacterDataComponent() const;


	UWidgetComponent* GetWidgetcomponent();

	UFUNCTION(BlueprintCallable, Category = "Character")
	UYogAbilitySystemComponent* GetASC() const;


	UFUNCTION()
	UBufferComponent* GetInputBufferComponent();

	UFUNCTION(BlueprintCallable)
	int32 GetStatePriority(EYogCharacterState State);


	//--------------------------------------------
	//	Data table for all character
	//--------------------------------------------

	// 基础通用技能集路径（代码固定，无需编辑器配置）
	// 对应 Content 资产：Content/Docs/GlobalSet/CharacterBaseSet/DA_Base_AbilitySet_Initial
	static constexpr const TCHAR* BaseAbilitySetPath = TEXT("/Game/Docs/GlobalSet/CharacterBaseSet/DA_Base_AbilitySet_Initial");

	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data")
	//TObjectPtr<UAbilityData> AbilityData;

	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data")
	//TObjectPtr<UGASTemplate> GasTemplate;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UAttributeStatComponent> AttributeStatsComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UGameEffectComponent> GameEffectComponent;






	//DELEGATE DEFINE
	UPROPERTY(BlueprintAssignable,Category = "Character|Attributes")
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

	UFUNCTION(BlueprintCallable)
	EWeaponState GetWeaponState();


	UFUNCTION(BlueprintCallable, Category = "Character|Attributes")
	void GetActiveAbilitiesWithTags(FGameplayTagContainer AbilityTags, TArray<UYogGameplayAbility*>& ActiveAbilities);



	UFUNCTION(BlueprintCallable, Category = "Character|Abilities")
	void GrantGameplayAbility(TSubclassOf<UYogGameplayAbility> AbilityToGrant, int32 AbilityLevel);


	UFUNCTION(BlueprintCallable, Category = "Character|Debug")
	void PrintAllGameplayTags(const FGameplayTagContainer& TagContainer);



	///** Map of gameplay tags to gameplay effect containers */
	//UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Character|Buff")
	//TMap<FGameplayTag, FYogGameplayEffectContainer> BufferMap;



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
	void UpdateCharacterState(EYogCharacterState newState);


public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsDead;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bMovable;


	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UBufferComponent> InputBufferComponent;


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
	friend URuneAttributeSet;
	friend UDamageAttributeSet;
	//friend UAdditionAttributeSet;

private:
	UPROPERTY()
	EYogCharacterState CurrentState;

	UPROPERTY()
	EYogCharacterState PreviousState;

	UPROPERTY()
	EWeaponState CurrentWeaponState;

	void InitializeComponentsWithStats(UCharacterData* characterData);
	
	void InitializeStats(const FYogBaseAttributeData* attributeData) const;

	void InitializeMovement(const FMovementData* movementData) const;
};



FORCEINLINE UAttributeStatComponent* AYogCharacterBase:: GetAttributeStatsComponent() const
{
	return AttributeStatsComponent;
}

FORCEINLINE UGameEffectComponent* AYogCharacterBase::GetGameEffectComponent() const
{
	return GameEffectComponent;
}

FORCEINLINE UBufferComponent* AYogCharacterBase::GetInputBufferComponent() const
{
	return InputBufferComponent;
}

FORCEINLINE UCharacterDataComponent* AYogCharacterBase::GetCharacterDataComponent() const
{
	return CharacterDataComponent;
}
