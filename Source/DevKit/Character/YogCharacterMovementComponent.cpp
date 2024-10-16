// Fill out your copyright notice in the Description page of Project Settings.


#include "YogCharacterMovementComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"

#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"


//UE_DEFINE_GAMEPLAY_TAG(TAG_Gameplay_MovementStopped, "Gameplay.MovementStopped");

UYogCharacterMovementComponent::UYogCharacterMovementComponent(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
}

FRotator UYogCharacterMovementComponent::GetDeltaRotation(float DeltaTime) const
{
	//if (UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner()))
	//{
	//	if (ASC->HasMatchingGameplayTag(TAG_Gameplay_MovementStopped))
	//	{
	//		return FRotator(0, 0, 0);
	//	}
	//}

	return Super::GetDeltaRotation(DeltaTime);
}

float UYogCharacterMovementComponent::GetMaxSpeed() const
{
	//if (UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner()))
	//{
	//	if (ASC->HasMatchingGameplayTag(TAG_Gameplay_MovementStopped))
	//	{
	//		return 0;
	//	}
	//}
	return Super::GetMaxSpeed();
}


void UYogCharacterMovementComponent::InitializeComponent()
{
	Super::InitializeComponent();
}
