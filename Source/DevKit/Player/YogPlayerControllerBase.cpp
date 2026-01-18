// Fill out your copyright notice in the Description page of Project Settings.


#include "YogPlayerControllerBase.h"
#include <AbilitySystemGlobals.h>
#include "../Character/YogCharacterBase.h"

#include "GameFramework/CharacterMovementComponent.h"
#include <DevKit/Camera/YogCameraPawn.h>
#include "Player/PlayerCharacterBase.h"
#include <EnhancedInputSubsystems.h>
#include "../Item/ItemSpawner.h"
#include "DevKit/SaveGame/YogSaveSubSystem.h"
#include "DevKit/System/YogGameInstanceBase.h"


void AYogPlayerControllerBase::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

}

void AYogPlayerControllerBase::OnUnPossess()
{
	Super::OnUnPossess();
}

UYogAbilitySystemComponent* AYogPlayerControllerBase::GetYogAbilitySystemComponent() const
{
	APawn* PossessdPawn = GetPawn();
	UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(PossessdPawn);
	return Cast<UYogAbilitySystemComponent>(ASC) ? Cast<UYogAbilitySystemComponent>(ASC) : nullptr;


}

void AYogPlayerControllerBase::SetEnableRotationRate(FRotator RotationRate, bool isEnable)
{
	if (isEnable)
	{
		
		AYogCharacterBase* OwnedCharacter =Cast<AYogCharacterBase>(this->GetPawn());
		OwnedCharacter->GetCharacterMovement()->RotationRate = RotationRate;
	}
	else
	{

	}
}


void AYogPlayerControllerBase::SpawnCameraPawn(APlayerCharacterBase* TargetCharacter) {
	//Get possessed character and spawn camera pawn attached on it
	if (IsValid(TargetCharacter))
	{
		FVector Location = TargetCharacter->GetActorLocation();
		FRotator Rotation = FRotator::ZeroRotator;
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		AYogCameraPawn* CameraActorPawn = GetWorld()->SpawnActor<AYogCameraPawn>(CameraPawnClass, Location, Rotation, SpawnParams);
		CameraActorPawn->SetOwner(TargetCharacter);
		TargetCharacter->SetOwnCamera(CameraActorPawn);
		this->SetViewTargetWithBlend(CameraActorPawn, 0.0f, EViewTargetBlendFunction::VTBlend_Linear, 0.0f, false);
	}


}

void AYogPlayerControllerBase::SetPlayerState(EYogCharacterState newState)
{

}

void AYogPlayerControllerBase::BeginPlay()
{
	Super::BeginPlay();

	
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(this->GetLocalPlayer()))
	{
		Subsystem->AddMappingContext(DefaultMappingContext, 0);
	}


	
	//AYogCharacterBase* TargetCharacter = Cast<AYogCharacterBase>(UGameplayStatics::GetPlayerCharacter(this, 0));
	
	SpawnCameraPawn(Cast<APlayerCharacterBase>(this->GetControlledCharacter()));

}

AYogCharacterBase* AYogPlayerControllerBase::GetControlledCharacter()
{
	AYogCharacterBase* MyCharacter = Cast<AYogCharacterBase>(GetPawn());
	return MyCharacter;
}

void AYogPlayerControllerBase::OnInteractTriggered()
{
	APlayerCharacterBase* player = Cast<APlayerCharacterBase>(this->GetPawn());


}

void AYogPlayerControllerBase::ToggleInput(bool bEnable)
{
	if (bEnable)
	{
		// Enable input
		EnableInput(this); // Re-enables input if disabled
		SetIgnoreMoveInput(false);
		SetIgnoreLookInput(false);
	}
	else
	{
		// Disable input
		DisableInput(this); // Disables all input
		// (Optional) Explicitly ignore move/look input
		SetIgnoreMoveInput(true);
		SetIgnoreLookInput(true);
	}

}

