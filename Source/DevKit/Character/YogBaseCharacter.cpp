// Fill out your copyright notice in the Description page of Project Settings.


#include "YogBaseCharacter.h"
#include "YogCharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include <DevKit/AbilitySystem/Attribute/YogCombatSet.h>
#include <DevKit/AbilitySystem/Attribute/YogHealthSet.h>
#include <DevKit/AbilitySystem/YogAbilitySystemComponent.h>

AYogBaseCharacter::AYogBaseCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UYogCharacterMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;

	AbilitySystemComponent = ObjectInitializer.CreateDefaultSubobject<UYogAbilitySystemComponent>(this, TEXT("AbilitySystemComponent"));

	UCapsuleComponent* CapsuleComp = GetCapsuleComponent();
	check(CapsuleComp);
	CapsuleComp->InitCapsuleSize(40.0f, 90.0f);

	HealthSet = CreateDefaultSubobject<UYogHealthSet>(TEXT("HealthSet"));
	CombatSet = CreateDefaultSubobject<UYogCombatSet>(TEXT("CombatSet"));
}

UYogAbilitySystemComponent* AYogBaseCharacter::GetASC() const
{
	return AbilitySystemComponent;
}
