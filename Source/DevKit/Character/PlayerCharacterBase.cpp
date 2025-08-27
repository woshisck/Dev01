// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerCharacterBase.h"
#include "YogCharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "../Item/ItemInstance.h"
#include "Kismet/KismetSystemLibrary.h"
#include "../Inventory/InventoryManagerComponent.h"
#include "../AbilitySystem/Abilities/YogGameplayAbility.h"
#include <DevKit/Player/YogPlayerControllerBase.h>
#include "../Camera/YogCameraPawn.h"
#include <DevKit/AbilitySystem/YogAbilitySystemComponent.h>
#include "../Buff/Aura/AuraBase.h"

APlayerCharacterBase::APlayerCharacterBase(const FObjectInitializer& ObjectInitializer)
	//: Super(ObjectInitializer.SetDefaultSubobjectClass<UYogCharacterMovementComponent>(ACharacter::CharacterMovementComponentName))
	: Super(ObjectInitializer)
{

	PlayerAttributeSet = CreateDefaultSubobject<UPlayerAttributeSet>(TEXT("PlayerAttributeSet"));

}

void APlayerCharacterBase::SetOwnCamera(AYogCameraPawn* cameraActor)
{

	CameraPawnActor = cameraActor;
}

AYogCameraPawn* APlayerCharacterBase::GetOwnCamera()
{

	return CameraPawnActor;
}

void APlayerCharacterBase::SetPrepareItem(AActor* actor)
{
	temp_Item_prepare = actor;
	UE_LOG(LogTemp, Warning, TEXT("temp_Item_prepare set: %s"), *temp_Item_prepare->GetName());
}

void APlayerCharacterBase::DropPrepareItem()
{
	temp_Item_prepare = nullptr;
}

AActor* APlayerCharacterBase::GetPrepareItem()
{
	return temp_Item_prepare;
}

void APlayerCharacterBase::Die()
{
	Super::Die();

}



void APlayerCharacterBase::BeginPlay()
{
	Super::BeginPlay();


	this->CurrentState = EYogCharacterState::Idle;
}

void APlayerCharacterBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

}


