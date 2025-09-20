// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/PlayerCharacterBase.h"
#include "Character/YogCharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "../Item/ItemInstance.h"
#include "Kismet/KismetSystemLibrary.h"
#include "../Inventory/InventoryManagerComponent.h"
#include "../AbilitySystem/Abilities/YogGameplayAbility.h"
#include <DevKit/Player/YogPlayerControllerBase.h>
#include "../Camera/YogCameraPawn.h"
#include <DevKit/AbilitySystem/YogAbilitySystemComponent.h>
#include "../Buff/Aura/AuraBase.h"
#include "DevKit/SaveGame/YogSaveGame.h"
#include "UObject/ConstructorHelpers.h"

APlayerCharacterBase::APlayerCharacterBase(const FObjectInitializer& ObjectInitializer)
	//: Super(ObjectInitializer.SetDefaultSubobjectClass<UYogCharacterMovementComponent>(ACharacter::CharacterMovementComponentName))
	: Super(ObjectInitializer)
{

	UObject* loadObj = StaticLoadObject(UBlueprint::StaticClass(), NULL, TEXT("Blueprint'/Game/Code/Characters/B_PlayerBase.B_PlayerBase_C'"));
	if (loadObj != nullptr)
	{
		PlayerBlueprintClass = loadObj->GetClass();
		//UBlueprint* ubp = Cast<UBlueprint>(loadObj);
		//AActor* spawnActor = GetWorld()->SpawnActor<AActor>(ubp->GeneratedClass);
		//UE_LOG(LogClass, Log, TEXT("Success"));
	}


	//static ConstructorHelpers::FClassFinder<APlayerCharacterBase> BlueprintFinder(TEXT("Blueprint'/Game/Code/Characters/B_PlayerBase.B_PlayerBase_C'"));
	//if (BlueprintFinder.Class)
	//{
	//	PlayerBlueprintClass = BlueprintFinder.Class;
	//}

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


