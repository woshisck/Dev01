// Fill out your copyright notice in the Description page of Project Settings.


#include "CardComponent.h"
#include "GameplayEffect.h"
#include <Algo/RandomShuffle.h>

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
	for (int i = 0; i < DeckSize; i++)
	{
		MoveItemAtIndex(UPARAM(ref) CardPool, UPARAM(ref) CardDeck, i);
		//FCardProperty cardPorpertyConfig = card_data_pool->CardProperties[FMath::RandRange(0, card_data_pool->CardProperties.Num() - 1)];

		//CardPool.Add(cardPorpertyConfig);
	}


	//for (const FCardProperty& cardproperty : CardPool)
	//{
	//	FCardProperty cardPorpertyConfig = card_data_pool->CardProperties[FMath::RandRange(0, card_data_pool->CardProperties.Num() - 1)];

	//	CardPool.Add(cardPorpertyConfig);
	//}
}

void UCardComponent::Pop()
{
	Event_OnCardPopSignature.Broadcast();
}

void UCardComponent::Shuffle(UPARAM(ref) TArray<FCardProperty>& cards)
{

	Algo::RandomShuffle(cards); // Fastest and cleanest

	Event_OnCardShuffleSignature.Broadcast();
}

void UCardComponent::FillPool(UCardData* card_data_pool)
{
	for (int i = 0; i < CardPoolSize; i++)
	{
		FCardProperty cardPorpertyConfig = card_data_pool->CardProperties[FMath::RandRange(0, card_data_pool->CardProperties.Num() - 1)];

		CardPool.Add(cardPorpertyConfig);
	}
	//UE_LOG(LogTemp, Warning, TEXT(""));

}

void UCardComponent::FillPoolWithRareDistribution(UCardData* card_data_pool)
{
	
	for (int i = 0; i < CardPoolSize; i++)
	{
		int Rare = FMath::RandRange(1, 5);
		FCardProperty card_pick = card_data_pool->GetCardPropertyWithRareRandom(Rare);
		CardPool.Add(card_pick);
		//card_data_pool->CardProperties
	}

}

void UCardComponent::MoveCardAtIndex(UPARAM(ref)TArray<FCardProperty>& Source, UPARAM(ref)TArray<FCardProperty>& Dest, int32 index)
{
	if (Source.IsValidIndex(index))
	{
		Dest.Add(Source[index]);
		Source.RemoveAtSwap(index);      // Faster: swaps with last element
	}
}

void UCardComponent::PrintCardPool()
{
	UE_LOG(LogTemp, Warning, TEXT("Pool property:"));
	for (FCardProperty& cardproperty : CardPool)
	{

		FString cardInfo = cardproperty.CardPropertyToString(cardproperty);
		UE_LOG(LogTemp, Warning, TEXT("%s"), *cardInfo);
	}
}

void UCardComponent::PrintDeck()
{
	UE_LOG(LogTemp, Warning, TEXT("Deck property:"));
	for (FCardProperty& cardproperty : CardDeck)
	{
		
		FString cardInfo = cardproperty.CardPropertyToString(cardproperty);
		UE_LOG(LogTemp, Warning, TEXT("%s at index "), *cardInfo);
	}
}

/*
* 	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FCardProperty> CardDeck;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FCardProperty> CardPool;
*/
//

// Called when the game starts
void UCardComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}
