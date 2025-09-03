// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/YogCharacterMovementComponent.h"

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

const FYogGroundInfo& UYogCharacterMovementComponent::GetGroundInfo()
{
	
	if (MovementMode == MOVE_Walking)
	{
		CachedGroundInfo.GroundHitResult = CurrentFloor.HitResult;
		CachedGroundInfo.GroundDistance = 0.0f;
	}
	return CachedGroundInfo;

	// TODO: insert return statement here
}


void UYogCharacterMovementComponent::InitializeComponent()
{
	Super::InitializeComponent();
}
