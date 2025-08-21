// Fill out your copyright notice in the Description page of Project Settings.


#include "EnemyCharacterBase.h"
#include <DevKit/AbilitySystem/YogAbilitySystemComponent.h>
#include "YogCharacterMovementComponent.h"

AEnemyCharacterBase::AEnemyCharacterBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UYogCharacterMovementComponent>(ACharacter::CharacterMovementComponentName))
{
}

void AEnemyCharacterBase::BeginPlay()
{
	Super::BeginPlay();



}

void AEnemyCharacterBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
}

