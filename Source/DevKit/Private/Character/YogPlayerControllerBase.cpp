// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/YogPlayerControllerBase.h"
#include <AbilitySystemGlobals.h>
#include "Character/YogCharacterBase.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "Camera/YogCameraPawn.h"
#include "Character/PlayerCharacterBase.h"

#include "Character/YogCharacterBase.h"
#include <EnhancedInputSubsystems.h>
#include "Item/ItemSpawner.h"
#include "SaveGame/YogSaveSubsystem.h"
#include "System/YogGameInstanceBase.h"
#include "Kismet/KismetMathLibrary.h"

#include "Component/BufferComponent.h"



void AYogPlayerControllerBase::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	//UYogGameInstanceBase* GI = Cast<UYogGameInstanceBase>(GetGameInstance());

	//UGameInstance* GameInstancePtr = Cast<UGameInstance>(GetWorld()->GetGameInstance());
	//UYogSaveSubsystem* SaveSubsystem = GI->GetSubsystem<UYogSaveSubsystem>();

	//if (SaveSubsystem->CurrentSaveGame)
	//{
	//	SaveSubsystem->LoadSaveGame(SaveSubsystem->CurrentSaveGame);
	//}

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
	if (IsValid(TargetCharacter))
	{
		FVector Location = TargetCharacter->GetActorLocation();
		FRotator Rotation = FRotator::ZeroRotator;
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		AYogCameraPawn* CameraActorPawn = GetWorld()->SpawnActor<AYogCameraPawn>(CameraPawnClass, Location, Rotation, SpawnParams);
		CameraActorPawn->SetOwner(TargetCharacter);
		//TargetCharacter->SetOwnCamera(CameraActorPawn);
		this->SetViewTargetWithBlend(CameraActorPawn, 0.0f, EViewTargetBlendFunction::VTBlend_Linear, 0.0f, false);
	}


}

void AYogPlayerControllerBase::SetPlayerState(EYogCharacterState newState)
{

}

void AYogPlayerControllerBase::BeginPlay()
{
	Super::BeginPlay();


	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		Subsystem->AddMappingContext(DefaultMappingContext, 0);
	}


	
	//AYogCharacterBase* TargetCharacter = Cast<AYogCharacterBase>(UGameplayStatics::GetPlayerCharacter(this, 0));
	
	//TODO: Swap to camera manager in future, but NOT YET

	//SpawnCameraPawn(Cast<AYogCharacterBase>(this->GetControlledCharacter()));

}

void AYogPlayerControllerBase::SetupInputComponent()
{
	Super::SetupInputComponent();
	if (UEnhancedInputComponent* EnhancedInputComp = Cast<UEnhancedInputComponent>(InputComponent))
	{
		if (Input_MoveAction)
		{
			const FEnhancedInputActionEventBinding& moveBinding = EnhancedInputComp->BindAction(Input_MoveAction, ETriggerEvent::Triggered, this, &AYogPlayerControllerBase::Move);
			MoveInputHandle = moveBinding.GetHandle();
		}
		if (Input_LightAttack)
		{
			const FEnhancedInputActionEventBinding& lightAttackBinding = EnhancedInputComp->BindAction(Input_LightAttack, ETriggerEvent::Triggered, this, &AYogPlayerControllerBase::LightAtack);
			LightAttackInputHandle = lightAttackBinding.GetHandle();
		}
		if (Input_HeavyAttack)
		{
			const FEnhancedInputActionEventBinding& heavyAttackBinding = EnhancedInputComp->BindAction(Input_HeavyAttack, ETriggerEvent::Triggered, this, &AYogPlayerControllerBase::HeavyAtack);
			HeavyAttackInputHandle = heavyAttackBinding.GetHandle();
		}
		if (Input_Dash)
		{
			const FEnhancedInputActionEventBinding& dashBinding = EnhancedInputComp->BindAction(Input_Dash, ETriggerEvent::Triggered, this, &AYogPlayerControllerBase::Dash);
			DashInputHandle = dashBinding.GetHandle();
		}
	}

}

AYogCharacterBase* AYogPlayerControllerBase::GetControlledCharacter()
{
	AYogCharacterBase* MyCharacter = Cast<AYogCharacterBase>(GetPawn());
	return MyCharacter;
}

void AYogPlayerControllerBase::OnInteractTriggered(const AItemSpawner* item)
{
	APlayerCharacterBase* player = Cast<APlayerCharacterBase>(this->GetPawn());

	UE_LOG(LogTemp, Warning, TEXT("item"));
}

void AYogPlayerControllerBase::LightAtack(const FInputActionValue& Value)
{
	if (APlayerCharacterBase* player = Cast<APlayerCharacterBase>(this->GetPawn()))
	{
		player->GetInputBufferComponent()->RecordLightAttack();
	}
	UE_LOG(LogTemp, Log, TEXT("LightAtack"));
}
void AYogPlayerControllerBase::HeavyAtack(const FInputActionValue& Value)
{
	if (APlayerCharacterBase* player = Cast<APlayerCharacterBase>(this->GetPawn()))
	{
		player->GetInputBufferComponent()->RecordHeavyAttack();
	}
	UE_LOG(LogTemp, Log, TEXT("HeavyAtack"));
}


void AYogPlayerControllerBase::Dash(const FInputActionValue& Value)
{
	if (APlayerCharacterBase* player = Cast<APlayerCharacterBase>(this->GetPawn()))
	{
		player->GetInputBufferComponent()->RecordHeavyAttack();
	}
	UE_LOG(LogTemp, Log, TEXT("Dash"));
}

void AYogPlayerControllerBase::Move(const FInputActionValue& Value)
{
	// Get the movement vector (2D: X = forward/back, Y = right/left)
	FVector2D Input = Value.Get<FVector2D>();
	FVector2D Rotated = Input.GetRotated(-45.0f);


	FVector MoveDir(Rotated.X, Rotated.Y, 0.0f);

	if (!MoveDir.IsNearlyZero())
	{
		// Get yaw-only rotation from the vector
		FRotator DesiredRotation = MoveDir.Rotation();

		if (APawn* ControlledPawn = GetPawn())
		{
			// Option A: snap instantly
			//ControlledPawn->SetActorRotation(DesiredRotation);

			 //Option B: smooth interpolation
			 //FRotator Current = ControlledPawn->GetActorRotation();
			 //FRotator NewRotation = FMath::RInterpTo(Current, DesiredRotation, GetWorld()->GetDeltaSeconds(), 10.0f);
			 //ControlledPawn->SetActorRotation(NewRotation);

			// Apply movement
			ControlledPawn->AddMovementInput(MoveDir.GetSafeNormal(), 1.0f);
		}
	}

	if (APlayerCharacterBase* player = Cast<APlayerCharacterBase>(this->GetPawn()))
	{
		player->GetInputBufferComponent()->RecordHeavyAttack();
	}

	//const FRotator playerTowards = UKismetMathLibrary::Conv_VectorToRotator(FVector(Rotated, 0));

	//SetControlRotation(playerTowards);

	////FRotator UKismetMathLibrary::Conv_VectorToRotator(FVector InVec)
	//if (APawn* ControlledPawn = GetPawn())
	//{
	//	
	//	//void AController::SetControlRotation(const FRotator & NewRotation)
	//	// Add movement input (forward/right)
	//	ControlledPawn->AddMovementInput(ControlledPawn->GetActorForwardVector(), Rotated.X);
	//	ControlledPawn->AddMovementInput(ControlledPawn->GetActorRightVector(), Rotated.Y);
	//}
	UE_LOG(LogTemp, Log, TEXT("Move"));
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

