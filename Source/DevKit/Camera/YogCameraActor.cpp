// Fill out your copyright notice in the Description page of Project Settings.



//#include "GameFramework/PawnMovementComponent.h"
#include "GameFramework/FloatingPawnMovement.h"

#include "YogCameraActor.h"



// Sets default values
AYogCameraActor::AYogCameraActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

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

			MovementComp->MaxSpeed = MovementData->MaxSpeed;
			MovementComp->Acceleration = MovementData->Acceleration;
			MovementComp->Deceleration = MovementData->Deceleration;
			MovementComp->TurningBoost = MovementData->TurningBoost;
		}

	}


}

// Called every frame
void AYogCameraActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

