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

FCardProperty UCardComponent::GetWeightedRandomCard(const TArray<FCardProperty>& card_properties)
{
	// Calculate total weight (sum of all Rare values)
	int32 TotalWeight = 0;
	for (const FCardProperty& Card : card_properties)
	{
		TotalWeight += Card.Rare;
	}

	// Pick a random number in [1, TotalWeight]
	int32 RandomWeight = FMath::RandRange(1, TotalWeight);

	// Walk through the list until we find the card
	int32 AccumulatedWeight = 0;
	for (const FCardProperty& Card : card_properties)
	{
		AccumulatedWeight += Card.Rare;
		if (RandomWeight <= AccumulatedWeight)
		{
			return Card;
		}
	}

	// Fallback (should never hit if weights are valid)
	return card_properties.Last();
}

void UCardComponent::Additem()
{
	FCardProperty card_property;
	Event_OnCardAddSignature.Broadcast(card_property);
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

FCardProperty UCardComponent::PopAtFirst()
{
	if (CardPool.Num() > 0)
	{
		FCardProperty item = CardPool[0];
		CardPool.RemoveAt(0);
		Event_OnCardPopSignature.Broadcast();
		return item;
	}
	else
	{
		FillPool(CardData);
		FCardProperty item = CardPool[0];
		CardPool.RemoveAt(0);
		Event_OnCardPopSignature.Broadcast();
		return item;
	}

}

void UCardComponent::Shuffle(UPARAM(ref) TArray<FCardProperty>& cards)
{

	Algo::RandomShuffle(cards); // Fastest and cleanest

	Event_OnCardShuffleSignature.Broadcast();
}

void UCardComponent::FillPool(UCardData* card_data_pool)
{
	CardPool.Empty();
	if (!card_data_pool || !CardData)
	{
		return;
	}
	else
	{
		for (int i = 0; i < CardPoolSize; i++)
		{
			FCardProperty SelectedCard = GetWeightedRandomCard(CardData->CardProperties);
			CardPool.Add(SelectedCard);
		}
	}
	//UE_LOG(LogTemp, Warning, TEXT(""));
	Event_OnCardFillPoolSignature.Broadcast(CardPool.Num());
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
}

void UCardComponent::PrintDeck()
{
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
