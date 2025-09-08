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


void APlayerCharacterBase::Die()
{
	Super::Die();

}

void APlayerCharacterBase::SavePlayer_Implementation(UYogSaveGame* SaveObject)
{
	if (SaveObject)
	{
		// Gather all relevant data for player
		FPlayerSaveData SaveData;
		//SaveData.Credits = Credits;
		//SaveData.PersonalRecordTime = PersonalRecordTime;
		//// Stored as FString for simplicity (original Steam ID is uint64)
		//SaveData.PlayerID = GetUniqueId().ToString();

		// May not be alive while we save

		SaveData.Location = this->GetActorLocation();
		SaveData.Rotation = this->GetActorRotation();
		SaveData.bResumeAtTransform = true;


		SaveObject->SavedPlayers.Add(SaveData);
	}
}

void APlayerCharacterBase::LoadPlayer_Implementation(UYogSaveGame* SaveObject)
{
	//if (SaveObject)
	//{
	//	FPlayerSaveData* FoundData = SaveObject->GetPlayerData(this);
	//	if (FoundData)
	//	{
	//		// Makes sure we trigger credits changed event
	//		AddCredits(FoundData->Credits);

	//		PersonalRecordTime = FoundData->PersonalRecordTime;
	//	}
	//	else
	//	{
	//		UE_LOGFMT(LogGame, Warning, "Could not find SaveGame data for player id: {playerid}.", GetPlayerId());
	//	}
	//}
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


