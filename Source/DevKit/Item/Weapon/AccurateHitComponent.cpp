// Fill out your copyright notice in the Description page of Project Settings.


#include "AccurateHitComponent.h"

// Sets default values for this component's properties
UAccurateHitComponent::UAccurateHitComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UAccurateHitComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


// Called every frame
void UAccurateHitComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
	if (TriggerTrace)
	{
		TArray<FHitResult> OutHits;
		//FVector* Start = LastKnownSocketLocation.Find(Socket3);
		//FVector End = MyPrimitive->GetSocketLocation(Socket1);

		AddHitResult(OutHits);
	}


}

void UAccurateHitComponent::AddHitResult(TArray<FHitResult> HitArrayToAdd)
{
}

