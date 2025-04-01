// Fill out your copyright notice in the Description page of Project Settings.


#include "YogPlayerControllerBase.h"
#include <AbilitySystemGlobals.h>
#include "../Character/YogCharacterBase.h"

#include "GameFramework/CharacterMovementComponent.h"
#include <DevKit/Camera/YogCameraActor.h>




void AYogPlayerControllerBase::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	AYogCharacterBase* PossessedCharacter = Cast<AYogCharacterBase>(InPawn);

	FVector Location = PossessedCharacter->GetActorLocation();
	
	FRotator Rotation = FRotator::ZeroRotator;
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	AYogCameraActor* CameraActor = GetWorld()->SpawnActor<AYogCameraActor>(AYogCameraActor::StaticClass(), Location, Rotation, SpawnParams);

	FAttachmentTransformRules AttachRules(EAttachmentRule::SnapToTarget, EAttachmentRule::SnapToTarget, EAttachmentRule::KeepWorld, false);
	CameraActor->AttachToActor(PossessedCharacter, AttachRules);
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
