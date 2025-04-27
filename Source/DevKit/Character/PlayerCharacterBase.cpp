// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerCharacterBase.h"



#include "YogCharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "../Item/ItemInstance.h"
#include "Kismet/KismetSystemLibrary.h"
#include "../Inventory/InventoryManagerComponent.h"
#include "../AbilitySystem/Abilities/YogGameplayAbility.h"
#include <DevKit/Player/YogPlayerControllerBase.h>

#include <DevKit/AbilitySystem/YogAbilitySystemComponent.h>


APlayerCharacterBase::APlayerCharacterBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UYogCharacterMovementComponent>(ACharacter::CharacterMovementComponentName))
{

}


void APlayerCharacterBase::BeginPlay()
{
	Super::BeginPlay();

	if (CharacterMovementDataTable)
	{
		static const FString ContextString(TEXT("Character movement Data Lookup"));
		FName RowName(TEXT("TripleC_Lvl_1")); // Name of the row you want to access
		FCharacterMovementData* MovementData = this->CharacterMovementDataTable->FindRow<FCharacterMovementData>(FName(TEXT("CharacterMoveLvl_1")), ContextString, true);

		if (MovementData)
		{
			/*
			UPROPERTY(EditAnywhere, BlueprintReadWrite)
			float MaxWalkSpeed;

			UPROPERTY(EditAnywhere, BlueprintReadWrite)
			float GroundFriction;

			UPROPERTY(EditAnywhere, BlueprintReadWrite)
			float BreakingDeceleration;

			UPROPERTY(EditAnywhere, BlueprintReadWrite)
			float MaxAcceleration;

			UPROPERTY(EditAnywhere, BlueprintReadWrite)
			FVector RotationRate;
			*/

			UYogCharacterMovementComponent* MovementComp = CastChecked<UYogCharacterMovementComponent>(GetCharacterMovement());
			MovementComp->MaxWalkSpeed = MovementData->MaxWalkSpeed;
			MovementComp->GroundFriction = MovementData->GroundFriction;
			MovementComp->MaxAcceleration = MovementData->MaxAcceleration;
			MovementComp->RotationRate = MovementData->RotationRate;
		}

	}


	if (AbilitySystemComponent) {

		HealthChangedDelegateHandle = AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AttributeSet->GetHealthAttribute()).AddUObject(this, &AYogCharacterBase::HealthChanged);
		MaxHealthChangedDelegateHandle = AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AttributeSet->GetMaxHealthAttribute()).AddUObject(this, &AYogCharacterBase::MaxHealthChanged);
		BaseDMGChangedDelegateHandle = AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AttributeSet->GetBaseDMGAttribute()).AddUObject(this, &AYogCharacterBase::BaseDMGChanged);
		WeaponDMGChangedDelegateHandle = AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AttributeSet->GetWeaponDMGAttribute()).AddUObject(this, &AYogCharacterBase::WeaponDMGChanged);
		BuffAmplifyChangedDelegateHandle = AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AttributeSet->GetBuffAmplifyAttribute()).AddUObject(this, &AYogCharacterBase::BuffAmplifyChanged);
	}

	this->CurrentState = EYogCharacterState::Idle;
}

void APlayerCharacterBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

}

