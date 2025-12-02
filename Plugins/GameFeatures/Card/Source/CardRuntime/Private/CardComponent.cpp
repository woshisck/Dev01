// Fill out your copyright notice in the Description page of Project Settings.


#include "CardComponent.h"
#include "GameplayEffect.h"

// Sets default values for this component's properties
UCardComponent::UCardComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	// ...
}

void UCardComponent::Additem()
{
}

void UCardComponent::RemoveItem()
{
}

void UCardComponent::InitDeck()
{
}

void UCardComponent::Pop()
{
	Event_OnCardPopSignature.Broadcast();
}

void UCardComponent::Shuffle()
{
	Event_OnCardShuffleSignature.Broadcast();
}


// Called when the game starts
void UCardComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}
