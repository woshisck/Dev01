// Fill out your copyright notice in the Description page of Project Settings.


#include "YogGameMode.h"

AYogGameMode::AYogGameMode(const FObjectInitializer& ObjectInitializer)
{
}

void AYogGameMode::OnGameRuleLoaded(const UYogGameRule* CurrentGameRule)
{
	for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
	{
		APlayerController* PC = Cast<APlayerController>(*Iterator);
		if ((PC != nullptr) && (PC->GetPawn() == nullptr))
		{
			if (PlayerCanRestart(PC))
			{
				RestartPlayer(PC);
			}
		}
	}
}
