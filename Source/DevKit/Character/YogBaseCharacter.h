// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ModularCharacter.h"

#include "YogBaseCharacter.generated.h"

class UYogAbilitySystemComponent;
class UYogHealthSet;
class UYogCombatSet;
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

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "AblitySystemComp")
	TObjectPtr<UYogAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<const class UYogHealthSet> HealthSet;
	// Combat attribute set used by this actor.
	UPROPERTY()
	TObjectPtr<const class UYogCombatSet> CombatSet;

};
