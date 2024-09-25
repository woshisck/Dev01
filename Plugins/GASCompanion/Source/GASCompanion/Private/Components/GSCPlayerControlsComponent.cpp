// Copyright 2021 Mickael Daniel. All Rights Reserved.

#include "Components/GSCPlayerControlsComponent.h"

#include "EnhancedInputSubsystems.h"
#include "GSCLog.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"

void UGSCPlayerControlsComponent::OnRegister()
{
	Super::OnRegister();

	const UWorld* World = GetWorld();
	APawn* MyOwner = GetPawn<APawn>();

	if (ensure(MyOwner) && World->IsGameWorld())
	{
		// Sign up for possess / un possess events so that we can release / setup input component
		if (!MyOwner->ReceiveRestartedDelegate.Contains(this, TEXT("OnPawnRestarted")))
		{
			MyOwner->ReceiveRestartedDelegate.AddDynamic(this, &UGSCPlayerControlsComponent::OnPawnRestarted);
		}

		if (!MyOwner->ReceiveControllerChangedDelegate.Contains(this, TEXT("OnControllerChanged")))
		{
			MyOwner->ReceiveControllerChangedDelegate.AddDynamic(this, &UGSCPlayerControlsComponent::OnControllerChanged);
		}

		// If our pawn has an input component we were added after restart
		if (MyOwner->InputComponent)
		{
			OnPawnRestarted(MyOwner);
		}
	}
}

void UGSCPlayerControlsComponent::OnUnregister()
{
	const UWorld* World = GetWorld();
	if (World && World->IsGameWorld())
	{
		ReleaseInputComponent();

		APawn* MyOwner = GetPawn<APawn>();
		if (MyOwner)
		{
			MyOwner->ReceiveRestartedDelegate.RemoveAll(this);
			MyOwner->ReceiveControllerChangedDelegate.RemoveAll(this);
		}
	}

	Super::OnUnregister();
}

void UGSCPlayerControlsComponent::SetupPlayerControls_Implementation(UEnhancedInputComponent* PlayerInputComponent)
{
}

void UGSCPlayerControlsComponent::TeardownPlayerControls_Implementation(UEnhancedInputComponent* PlayerInputComponent)
{
}

void UGSCPlayerControlsComponent::OnPawnRestarted(APawn* Pawn)
{
	GSC_LOG(Verbose, TEXT("UGSCPlayerControlsComponent::OnPawnRestarted Pawn: %s"), Pawn ? *Pawn->GetName() : TEXT("NONE"))
	if (ensure(Pawn && Pawn == GetOwner()) && Pawn->InputComponent)
	{
		ReleaseInputComponent();

		if (Pawn->InputComponent)
		{
			SetupInputComponent(Pawn);
		}
	}
}

void UGSCPlayerControlsComponent::OnControllerChanged(APawn* Pawn, AController* OldController, AController* NewController)
{
	GSC_LOG(Verbose, TEXT("UGSCPlayerControlsComponent::OnControllerChanged Pawn: %s"), Pawn ? *Pawn->GetName() : TEXT("NONE"))
	// Only handle releasing, restart is a better time to handle binding
	if (ensure(Pawn && Pawn == GetOwner()) && OldController)
	{
		ReleaseInputComponent(OldController);
	}
}

void UGSCPlayerControlsComponent::SetupInputComponent(APawn* Pawn)
{
	InputComponent = Cast<UEnhancedInputComponent>(Pawn->InputComponent);

	if (ensureMsgf(InputComponent, TEXT("Project must use EnhancedInputComponent to support PlayerControlsComponent")))
	{
		UEnhancedInputLocalPlayerSubsystem* Subsystem = GetEnhancedInputSubsystem();

		if (Subsystem && InputMappingContext)
		{
			Subsystem->AddMappingContext(InputMappingContext, InputPriority);
		}

		GSC_LOG(Verbose, TEXT("UGSCPlayerControlsComponent::SetupInputComponent Pawn: %s"), Pawn ? *Pawn->GetName() : TEXT("NONE"))
		SetupPlayerControls(InputComponent);
	}
}

void UGSCPlayerControlsComponent::ReleaseInputComponent(AController* OldController)
{
	UEnhancedInputLocalPlayerSubsystem* Subsystem = GetEnhancedInputSubsystem(OldController);
	if (Subsystem && InputComponent)
	{
		TeardownPlayerControls(InputComponent);

		if (InputMappingContext)
		{
			Subsystem->RemoveMappingContext(InputMappingContext);
		}
	}
	InputComponent = nullptr;
}

UEnhancedInputLocalPlayerSubsystem* UGSCPlayerControlsComponent::GetEnhancedInputSubsystem(AController* OldController) const
{
	// Fail safe check that this component was added to pawn, and avoid CastChecked if component was added to a non pawn
	// Might happen with Game Feature trying to add ability input binding on a PlayerState
	if (!GetPawn<APawn>())
	{
		return nullptr;
	}
	
	const APlayerController* PC = GetController<APlayerController>();
	if (!PC)
	{
		PC = Cast<APlayerController>(OldController);
		if (!PC)
		{
			return nullptr;
		}
	}

	const ULocalPlayer* LP = PC->GetLocalPlayer();
	if (!LP)
	{
		return nullptr;
	}

	return LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
}
