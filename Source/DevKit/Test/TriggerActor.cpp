// Fill out your copyright notice in the Description page of Project Settings.



#include "TriggerActor.h"

// Sets default values
ATriggerActor::ATriggerActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void ATriggerActor::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ATriggerActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
    if(bStartTickMove)
    {
        float CurrentFPS = 1.0f / GetWorld()->GetDeltaSeconds();

        float TargetFPS = 60.0f; // Target FPS, can be adjusted
        CurrentFPS = 1.0f / DeltaTime;
        float SpeedAdjustment = CurrentFPS / TargetFPS;

        // Example movement vector
        FVector Movement(100.0f * DeltaTime * SpeedAdjustment, 0.0f, 0.0f);

        // Update actor location
        SetActorLocation(GetActorLocation() + Movement);
    }


}


