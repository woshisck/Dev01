// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ModularCharacter.h"
#include <DevKit/AbilitySystem/Attribute/BaseAttributeSet.h>

#include "YogBaseCharacter.generated.h"


class UYogAbilitySystemComponent;

/**
 * 
 */
UCLASS()
class DEVKIT_API AYogBaseCharacter : public AModularCharacter
{
	GENERATED_BODY()
public:
	AYogBaseCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());



	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsDead;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bCanMove;


	virtual UYogAbilitySystemComponent* GetASC() const;


	UPROPERTY()
	TObjectPtr<const class UBaseAttributeSet> AttributeSet;


	//SKill
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Abilities)
	TArray<TSubclassOf<UGameplayEffect>> PassiveGameplayEffects;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "AblitySystemComp")
	TObjectPtr<UYogAbilitySystemComponent> AbilitySystemComponent;

	UFUNCTION(BlueprintImplementableEvent)
	void OnHealthChanged(float DeltaValue, const struct FGameplayTagContainer& EventTags);

	UFUNCTION(BlueprintImplementableEvent)
	void OnDamaged(float DamageAmount, const FHitResult& HitInfo, const struct FGameplayTagContainer& DamageTags, AYogBaseCharacter* InstigatorCharacter, AActor* DamageCauser);



	/** Apply the startup gameplay abilities and effects */
	void AddStartupGameplayAbilities();


	// Called from RPGAttributeSet, these call BP events above
	virtual void HandleDamage(float DamageAmount, const FHitResult& HitInfo, const struct FGameplayTagContainer& DamageTags, AYogBaseCharacter* InstigatorCharacter, AActor* DamageCauser);
	virtual void HandleHealthChanged(float DeltaValue, const struct FGameplayTagContainer& EventTags);

	virtual void HandleMoveSpeedChanged(float DeltaValue, const struct FGameplayTagContainer& EventTags);



	// Friended to allow access to handle functions above
	friend UBaseAttributeSet;
};
