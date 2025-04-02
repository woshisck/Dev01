// Fill out your copyright notice in the Description page of Project Settings.


#include "YogPlayerControllerBase.h"
#include <AbilitySystemGlobals.h>
#include "../Character/YogCharacterBase.h"

#include "GameFramework/CharacterMovementComponent.h"
#include <DevKit/Camera/YogCameraPawn.h>




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


void AYogPlayerControllerBase::SpawnCameraPawn(AYogCharacterBase* TargetCharacter) {
	//Get possessed character and spawn camera pawn attached on it

	FVector Location = TargetCharacter->GetActorLocation();
	FRotator Rotation = FRotator::ZeroRotator;
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	AYogCameraPawn* CameraActorPawn = GetWorld()->SpawnActor<AYogCameraPawn>(CameraPawnClass, Location, Rotation, SpawnParams);

	//FAttachmentTransformRules AttachRules(EAttachmentRule::SnapToTarget, EAttachmentRule::KeepWorld, EAttachmentRule::KeepWorld, false);
	//CameraActor->AttachToActor(TargetCharacter, AttachRules);

	//:SetViewTargetWithBlend(AActor* NewViewTarget, float BlendTime, EViewTargetBlendFunction BlendFunc, float BlendExp, bool bLockOutgoing)
	if (CameraActorPawn)
	{
		//TODO:: ADD custom camera controller
		//AAIController* AIController = GetWorld()->SpawnActor<AAIController>(AAIController::StaticClass(), Location, Rotation, SpawnParams);

		//if (AIController)
		//{
		//	// Possess the Pawn with the newly created Controller
		//	AIController->Possess(CameraActorPawn);
		//}

		this->SetViewTargetWithBlend(CameraActorPawn, 0.0f, EViewTargetBlendFunction::VTBlend_Linear, 0.0f, false);
	}

}

