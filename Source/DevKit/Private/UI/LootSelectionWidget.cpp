// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/LootSelectionWidget.h"
#include "GameModes/YogGameMode.h"
#include "Kismet/GameplayStatics.h"

void ULootSelectionWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 绑定 GameMode 的两个广播
	if (AYogGameMode* GM = Cast<AYogGameMode>(UGameplayStatics::GetGameMode(GetWorld())))
	{
		GM->OnLootGenerated.AddDynamic(this, &ULootSelectionWidget::HandleLootGenerated);
		GM->OnPhaseChanged.AddDynamic(this, &ULootSelectionWidget::HandlePhaseChanged);
	}
}

void ULootSelectionWidget::NativeDestruct()
{
	if (AYogGameMode* GM = Cast<AYogGameMode>(UGameplayStatics::GetGameMode(GetWorld())))
	{
		GM->OnLootGenerated.RemoveDynamic(this, &ULootSelectionWidget::HandleLootGenerated);
		GM->OnPhaseChanged.RemoveDynamic(this, &ULootSelectionWidget::HandlePhaseChanged);
	}

	Super::NativeDestruct();
}

void ULootSelectionWidget::HandleLootGenerated(const TArray<FLootOption>& LootOptions)
{
	CurrentLootOptions = LootOptions;
	OnLootOptionsReady(LootOptions);
}

void ULootSelectionWidget::HandlePhaseChanged(ELevelPhase NewPhase)
{
	OnLevelPhaseChanged(NewPhase);
}

void ULootSelectionWidget::SelectRuneLoot(int32 Index)
{
	if (AYogGameMode* GM = Cast<AYogGameMode>(UGameplayStatics::GetGameMode(GetWorld())))
	{
		GM->SelectLoot(Index);
	}
}

void ULootSelectionWidget::ConfirmAndTransition()
{
	if (AYogGameMode* GM = Cast<AYogGameMode>(UGameplayStatics::GetGameMode(GetWorld())))
	{
		GM->ConfirmArrangementAndTransition();
	}
}
