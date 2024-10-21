// Fill out your copyright notice in the Description page of Project Settings.


#include "YogBaseCharacter.h"
#include "YogCharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"


AYogBaseCharacter::AYogBaseCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UYogCharacterMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;

	UCapsuleComponent* CapsuleComp = GetCapsuleComponent();
	check(CapsuleComp);
	CapsuleComp->InitCapsuleSize(40.0f, 90.0f);

}
