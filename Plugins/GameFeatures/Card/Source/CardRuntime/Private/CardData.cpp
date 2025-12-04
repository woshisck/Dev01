// Fill out your copyright notice in the Description page of Project Settings.


#include "CardData.h"
#include <Algo/RandomShuffle.h>

FCardProperty UCardData::GetCardPropertyWithRareRandom(int target_rare)
{
	TArray<FCardProperty> result_array;
	for (const FCardProperty& cardproperty : CardProperties)
	{
		if (cardproperty.Rare == target_rare)
		{
			result_array.Add(cardproperty);
		}
	}
	Algo::RandomShuffle(result_array); // Fastest and cleanest
	return result_array[0];
}
