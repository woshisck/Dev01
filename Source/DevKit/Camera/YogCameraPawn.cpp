// Fill out your copyright notice in the Description page of Project Settings.

#include "YogCameraPawn.h"
//#include "GameFramework/PawnMovementComponent.h"
#include "GameFramework/FloatingPawnMovement.h"
#include <AIController.h>




// Sets default values
AYogCameraPawn::AYogCameraPawn(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{

 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	
	FloatingMovementComponent = ObjectInitializer.CreateDefaultSubobject<UFloatingPawnMovement>(this, TEXT("PawnFloatingMovementComp"));
	//AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	CameraStatus = EYogCameraStates::Idle;
}

void AYogCameraPawn::PostActorCreated()
{
	Super::PostActorCreated();

	//if (this->CameraStatus == EYogCameraStates::FocusCharacter)
	//{	//get distance from player < attached dist
	//	if (FocusCharacter)
	//	{
	//		FVector CurrentPlayerLoc = FocusCharacter->GetActorLocation();


	//		//Move self to Focus Character


	//	}
	//}

}


void AYogCameraPawn::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	if (NewController)
	{
		// Cast to specific controller type if needed
		APlayerController* PC = Cast<APlayerController>(NewController);
		if (PC)
		{
			// Player controller specific logic
			UE_LOG(LogTemp, Log, TEXT("Possessed by player controller!"));
		}
		else
		{
			// AI controller case
			AAIController* AIC = Cast<AAIController>(NewController);
			this->CameraController = AIC;
		}
	}

}

// Called when the game starts or when spawned
void AYogCameraPawn::BeginPlay()
{
	Super::BeginPlay();
	if (CameraMovementDataTable)
	{
		static const FString ContextString(TEXT("Camera movement Data Lookup"));
		FName RowName(TEXT("CameraMovement_Lvl_1")); // Name of the row you want to access
		FCameraMovementData* MovementData = this->CameraMovementDataTable->FindRow<FCameraMovementData>(FName(TEXT("CameraMovement_Lvl_1")), ContextString, true);

		if (MovementData)
		{

			UFloatingPawnMovement* MovementComp = CastChecked<UFloatingPawnMovement>(this->GetMovementComponent());

			
			//this->MaxSpeedCache = MovementData->MaxSpeed;
			//this->AccelerationCache = MovementData->Acceleration;
			//this->DecelerationCache = MovementData->Deceleration;
			//this->TurningBoostCache = MovementData->TurningBoost;


			this->cache_followSpeed = MovementData->FollowSpeed;
			this->cache_focusSpeed = MovementData->FocusSpeed;
			this->cache_distFromCharacter = MovementData->DistFromCharacter;

			//MovementComp->MaxSpeed = this->MaxSpeedCache;
			//MovementComp->Acceleration = this->AccelerationCache;
			//MovementComp->Deceleration = this->DecelerationCache;
			//MovementComp->TurningBoost = this->TurningBoostCache;

		}

	}



}

// Called every frame
void AYogCameraPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	

	/*
	  	FocusCharacter		UMETA(DisplayName = "FocusCharacter"),
		FollowMove			UMETA(DisplayName = "FollowMove"),
		Idle				UMETA(DisplayName = "Idle")
	 */
	
	if (CameraStatus == EYogCameraStates::FocusCharacter)
	{
		ACharacter* TargetCharacter = UGameplayStatics::GetPlayerCharacter(this, 0);
		FVector TargetLoc = TargetCharacter->GetActorLocation();


		FVector Loc = FMath::VInterpTo(this->GetActorLocation(), TargetLoc, DeltaTime, cache_focusSpeed); 
		this->SetActorLocation(Loc);
		/*UE_LOG(LogTemp, Warning, TEXT("TargetLoc value: %f, %f, %f"), TargetLoc.X, TargetLoc.Y, TargetLoc.Z);*/

	}
	if (CameraStatus == EYogCameraStates::FollowMove)
	{
		ACharacter* TargetCharacter = UGameplayStatics::GetPlayerCharacter(this, 0);
		PlayerTargetLoc = TargetCharacter->GetActorLocation() + cache_playerMovementInput * cache_distFromCharacter;

		FVector Loc = FMath::VInterpTo(this->GetActorLocation(), PlayerTargetLoc, DeltaTime, cache_followSpeed);
		this->SetActorLocation(Loc);


		
	}
	if (CameraStatus == EYogCameraStates::Idle)
	{
		//TODO:: camera idle tick ability NEED
	}


	//UPROPERTY(BlueprintReadOnly, Category = "Movement")
	//EYogCameraStates CameraStatus;


}

void AYogCameraPawn::SetCameraStates(EYogCameraStates NewMovementMode)
{






	PrevStatus = CameraStatus;
	CameraStatus = NewMovementMode;


	// Handle change in movement mode
	OnCameraStatesChanged(PrevStatus, NewMovementMode);


}





void AYogCameraPawn::OnCameraStatesChanged(EYogCameraStates PreviousMovementMode, EYogCameraStates NextMovementMode)
{
	if (NextMovementMode == EYogCameraStates::FocusCharacter)
	{

	}
	else
	{

	}

	if (NextMovementMode == EYogCameraStates::FollowMove)
	{

	}
	else
	{

	}

}


