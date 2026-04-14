// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/LootSelectionWidget.h"
#include "GameModes/YogGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "Character/YogPlayerControllerBase.h"

void ULootSelectionWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 允许接收键盘焦点，使 OnKeyDown 在蓝图中生效
	bIsFocusable = true;

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

	// 在 C++ 里直接显示并切换输入模式，不依赖蓝图 Cast
	SetVisibility(ESlateVisibility::Visible);
	if (AYogPlayerControllerBase* PC = Cast<AYogPlayerControllerBase>(GetOwningPlayer()))
	{
		PC->SetBlockGameInput(true, true);  // UIOnly：LMB 不被攻击消耗，按钮可点击
	}

	OnLootOptionsReady(LootOptions);  // 通知蓝图填充卡片数据
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

	// 选完后在 C++ 里直接隐藏并恢复输入
	SetVisibility(ESlateVisibility::Hidden);
	if (AYogPlayerControllerBase* PC = Cast<AYogPlayerControllerBase>(GetOwningPlayer()))
	{
		PC->SetBlockGameInput(false);  // 恢复游戏输入 + GameOnly + 隐藏鼠标
	}
}

void ULootSelectionWidget::ConfirmAndTransition()
{
	if (AYogGameMode* GM = Cast<AYogGameMode>(UGameplayStatics::GetGameMode(GetWorld())))
	{
		GM->ConfirmArrangementAndTransition();
	}
}
