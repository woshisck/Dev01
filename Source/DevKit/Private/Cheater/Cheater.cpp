// Fill out your copyright notice in the Description page of Project Settings.


#include "Cheater/Cheater.h"

// Sets default values
ACheater::ACheater()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void ACheater::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ACheater::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

