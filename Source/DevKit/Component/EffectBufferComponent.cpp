// Fill out your copyright notice in the Description page of Project Settings.


#include "EffectBufferComponent.h"
#include "DevKit/Buff/BuffElement.h"
#include "DevKit/AbilitySystem/GameplayEffect/YogGameplayEffect.h"

// Sets default values for this component's properties
UEffectBufferComponent::UEffectBufferComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	// ...
}


// Called when the game starts
void UEffectBufferComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}

TSubclassOf<UBuffElement> UEffectBufferComponent::GetItemAt(int index)
{
	return BuffArray[index];
}

void UEffectBufferComponent::MoveToNextItem()
{

	if (CurrentIndex == BuffArray.Num())
	{
		CurrentIndex = 0;
	}
	else
	{
		CurrentIndex++;
	}
}

bool UEffectBufferComponent::RemoveItemByIndex(int index)
{
	if (BuffArray.IsValidIndex(index))
	{
		BuffArray.RemoveAt(index);

		// Adjust current index if needed
		if (CurrentIndex >= BuffArray.Num() && BuffArray.Num() > 0)
		{
			CurrentIndex = BuffArray.Num() - 1;
		}
		else if (BuffArray.Num() == 0)
		{
			CurrentIndex = 0;
		}

		return true;
	}
	return false;
}

bool UEffectBufferComponent::RemoveItem(TSubclassOf<UBuffElement> BuffToRemove)
{
	if (BuffToRemove)
	{
		int32 RemovedCount = BuffArray.Remove(BuffToRemove);
		return RemovedCount > 0;
	}
	return false;
}

bool UEffectBufferComponent::AddItem(TSubclassOf<UBuffElement> buff)
{

	BuffArray.Add(buff);
	return true;

}

void UEffectBufferComponent::ClearAll()
{
	BuffArray.Empty();
	CurrentIndex = 0;
}

int UEffectBufferComponent::GetBuffCount()
{
	return BuffArray.Num();
}


