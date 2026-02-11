// Fill out your copyright notice in the Description page of Project Settings.


#include "Component/KingStairComponent.h"
#include "GameplayEffect.h"
#include <Algo/RandomShuffle.h>

// Sets default values for this component's properties
UKingStairComponent::UKingStairComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;
	// ...
}


void UKingStairComponent::InitGrid()
{
	ClearGrid();
	for (int i = 0; i < GridCol; i++)
	{
		FKingStairRow default2DRow = FKingStairRow(GridRow, i);
		KingStairGrid.Add(default2DRow);
	}
}

void UKingStairComponent::SetNodeEffect(int x, int y, TSubclassOf<UYogGameplayEffect> gameplayeffect, int32 level)
{
	KingStairGrid[x][y].setNodeEffect(gameplayeffect, level);
}

void UKingStairComponent::SelectNode(int x, int y)
{
	KingStairGrid[x][y].IsSelected = true;

}

void UKingStairComponent::ClearGrid()
{
	KingStairGrid.Empty();
}
