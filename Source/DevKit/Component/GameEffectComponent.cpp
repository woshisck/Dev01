// Fill out your copyright notice in the Description page of Project Settings.


#include "GameEffectComponent.h"

#include "DevKit/AbilitySystem/GameplayEffect/YogGameplayEffect.h"

// Sets default values for this component's properties
UGameEffectComponent::UGameEffectComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	// ...
}


// Called when the game starts
void UGameEffectComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}

TSubclassOf<UPlayEffectDefinition> UGameEffectComponent::GetItemAt(int index)
{
	return BuffArray[index];
}

void UGameEffectComponent::MoveToNextItem()
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

bool UGameEffectComponent::RemoveItemByIndex(int index)
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

bool UGameEffectComponent::RemoveItem(TSubclassOf<UPlayEffectDefinition> BuffToRemove)
{
	if (BuffToRemove)
	{
		int32 RemovedCount = BuffArray.Remove(BuffToRemove);
		return RemovedCount > 0;
	}
	return false;
}

bool UGameEffectComponent::AddItem(TSubclassOf<UPlayEffectDefinition> buff)
{

	BuffArray.Add(buff);
	return true;

}

void UGameEffectComponent::ClearAll()
{
	BuffArray.Empty();
	CurrentIndex = 0;
}

int UGameEffectComponent::GetBuffCount()
{
	return BuffArray.Num();
}


