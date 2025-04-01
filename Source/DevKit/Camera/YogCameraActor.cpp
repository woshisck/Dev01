// Fill out your copyright notice in the Description page of Project Settings.

#include "YogCameraActor.h"
//#include "GameFramework/PawnMovementComponent.h"
#include "GameFramework/FloatingPawnMovement.h"
#include <ModularAIController.h>




// Sets default values
AYogCameraActor::AYogCameraActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

}

// Called when the game starts or when spawned
void AYogCameraActor::BeginPlay()
{
	Super::BeginPlay();
	check(CameraMovementDataTable);
	if (CameraMovementDataTable)
	{
		static const FString ContextString(TEXT("Camera movement Data Lookup"));
		FName RowName(TEXT("CameraMovement_Lvl_1")); // Name of the row you want to access
		FCameraMovementData* MovementData = this->CameraMovementDataTable->FindRow<FCameraMovementData>(FName(TEXT("CameraMovement_Lvl_1")), ContextString, true);

		if (MovementData)
		{
			/*
			UPROPERTY(EditAnywhere, BlueprintReadWrite)
			float MaxSpeed;

			UPROPERTY(EditAnywhere, BlueprintReadWrite)
			float Acceleration;

			UPROPERTY(EditAnywhere, BlueprintReadWrite)
			float Deceleration;

			UPROPERTY(EditAnywhere, BlueprintReadWrite)
			float TurningBoost;
			*/

			UFloatingPawnMovement* MovementComp = CastChecked<UFloatingPawnMovement>(this->GetMovementComponent());

			this->MaxSpeedCache = MovementData->MaxSpeed;
			this->AccelerationCache = MovementData->Acceleration;
			this->DecelerationCache = MovementData->Deceleration;
			this->TurningBoostCache = MovementData->TurningBoost;
			this->FocusAccCache = MovementData->FocusAcc;

			MovementComp->MaxSpeed = this->MaxSpeedCache;
			MovementComp->Acceleration = this->AccelerationCache;
			MovementComp->Deceleration = this->DecelerationCache;
			MovementComp->TurningBoost = this->TurningBoostCache;


		}

	}


}

// Called every frame
void AYogCameraActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AYogCameraActor::CameraMove(EYogCharacterState State)
{
	//UFloatingPawnMovement* MovementComp = CastChecked<UFloatingPawnMovement>(this->GetMovementComponent());
	//AAIController* Controller = Cast<AAIController>(this->GetController());

	//if (Controller)
	//{
	//	switch (State)
	//	{
	//	case EYogCharacterState::Move:

	//		break;
	//	case EYogCharacterState::Idle:


	//		break;
	//	case EYogCharacterState::AbilityCast:


	//		break;
	//	default:
	//		break;
	//	}
	//}
	/*
	TObjectPtr<AYogPlayerControllerBase> PlayerController;
	TObjectPtr<AYogCharacterBase> PlayerCharacter;
	*/


}


