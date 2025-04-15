// Fill out your copyright notice in the Description page of Project Settings.


#include "YogPlayerControllerBase.h"
#include <AbilitySystemGlobals.h>
#include "../Character/YogCharacterBase.h"

#include "GameFramework/CharacterMovementComponent.h"
#include <DevKit/Camera/YogCameraPawn.h>
#include <EnhancedInputSubsystems.h>




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
	this->CameraPawnActor = CameraActorPawn;
	this->SetViewTargetWithBlend(CameraActorPawn, 0.0f, EViewTargetBlendFunction::VTBlend_Linear, 0.0f, false);

	//FAttachmentTransformRules AttachRules(EAttachmentRule::SnapToTarget, EAttachmentRule::KeepWorld, EAttachmentRule::KeepWorld, false);
	//CameraActor->AttachToActor(TargetCharacter, AttachRules);

	//:SetViewTargetWithBlend(AActor* NewViewTarget, float BlendTime, EViewTargetBlendFunction BlendFunc, float BlendExp, bool bLockOutgoing)
	//if (CameraActorPawn)
	//{
		//TODO:: ADD custom camera controller
		//AAIController* AIController = GetWorld()->SpawnActor<AAIController>(AAIController::StaticClass(), Location, Rotation, SpawnParams);

		//if (AIController)
		//{
		//	// Possess the Pawn with the newly created Controller
		//	AIController->Possess(CameraActorPawn);
		//}


	//}

}

void AYogPlayerControllerBase::SetPlayerState(EYogCharacterState newState)
{
	//FVector MoveCache;
	//AYogPlayerControllerBase* controller = Cast<AYogPlayerControllerBase>(GetController());
	//switch (newState)
	//{
	//case EYogCharacterState::Move:
	//{
	//	OnCharacterStateUpdate.Broadcast(CurrentState, MoveCache);
	//	break;
	//case EYogCharacterState::Idle:

	//	OnCharacterStateUpdate.Broadcast(CurrentState, MoveCache);
	//	break;
	//case EYogCharacterState::AbilityCast:

	//	OnCharacterStateUpdate.Broadcast(CurrentState, MoveCache);
	//	break;
	//}
	//default:
	//	MoveCache = FVector(0, 0, 0);
	//	break;
	//}
}

void AYogPlayerControllerBase::BeginPlay()
{
	Super::BeginPlay();

	
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(this->GetLocalPlayer()))
	{
		Subsystem->AddMappingContext(DefaultMappingContext, 0);
	}

	AYogCharacterBase* TargetCharacter = Cast<AYogCharacterBase>(UGameplayStatics::GetPlayerCharacter(this, 0));
	SpawnCameraPawn(TargetCharacter);

}

AYogCharacterBase* AYogPlayerControllerBase::GetPossCharacter()
{
	AYogCharacterBase* MyCharacter = Cast<AYogCharacterBase>(GetPawn());
	return MyCharacter;
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

