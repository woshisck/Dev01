// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ModularCharacter.h"
#include <DevKit/AbilitySystem/Attribute/BaseAttributeSet.h>
#include "AbilitySystemInterface.h"
#include "YogBaseCharacter.generated.h"


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



	UFUNCTION(BlueprintCallable, Category = "GASDocumentation|GDCharacter")
	virtual bool IsAlive() const;


	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsDead;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bCanMove;



	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UBaseAttributeSet> AttributeSet;


	//SKill
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Abilities)
	TArray<TSubclassOf<UGameplayEffect>> PassiveGameplayEffects;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "AblitySystemComp")
	TObjectPtr<UYogAbilitySystemComponent> AbilitySystemComponent;

	UFUNCTION(BlueprintImplementableEvent)
	void OnHealthChanged(float DeltaValue, const struct FGameplayTagContainer& EventTags);

	UFUNCTION(BlueprintImplementableEvent)
	void OnDamaged(float DamageAmount, const FHitResult& HitInfo, const struct FGameplayTagContainer& DamageTags, AYogBaseCharacter* InstigatorCharacter, AActor* DamageCauser);


	UFUNCTION(BlueprintCallable, Category = "Character|Attributes")
	float GetHealth() const;

	/** Apply the startup gameplay abilities and effects */
	void AddStartupGameplayAbilities();


	// Called from RPGAttributeSet, these call BP events above
	virtual void HandleDamage(float DamageAmount, const FHitResult& HitInfo, const struct FGameplayTagContainer& DamageTags, AYogBaseCharacter* InstigatorCharacter, AActor* DamageCauser);
	virtual void HandleHealthChanged(float DeltaValue, const struct FGameplayTagContainer& EventTags);

	virtual void HandleMoveSpeedChanged(float DeltaValue, const struct FGameplayTagContainer& EventTags);


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

	// Friended to allow access to handle functions above
	friend UBaseAttributeSet;
};
