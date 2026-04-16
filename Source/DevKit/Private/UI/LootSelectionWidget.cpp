// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/LootSelectionWidget.h"
#include "GameModes/YogGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "Character/YogPlayerControllerBase.h"
#include "Input/CommonUIInputTypes.h"

void ULootSelectionWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 未激活时折叠
	SetVisibility(ESlateVisibility::Collapsed);

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
	OnLootOptionsReady(LootOptions);  // 通知蓝图填充卡片数据

	// 通过 CommonUI 激活（SetVisibility / 暂停 / 输入模式由 NativeOnActivated 处理）
	ActivateWidget();
	CurrentHighlightIndex = 0;
	OnCardFocused(0);
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

	// 通过 CommonUI 停用（SetVisibility / 恢复暂停 / 恢复输入模式由 NativeOnDeactivated 处理）
	DeactivateWidget();
}

// ============================================================
//  CommonUI 生命周期
// ============================================================

TOptional<FUIInputConfig> ULootSelectionWidget::GetDesiredInputConfig() const
{
	// Menu（UIOnly）：鼠标完全交给 Slate，不会被攻击等 EnhancedInput 消耗
	return FUIInputConfig(ECommonInputMode::Menu, EMouseCaptureMode::NoCapture);
}

void ULootSelectionWidget::NativeOnActivated()
{
	Super::NativeOnActivated();
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	if (APlayerController* PC = GetOwningPlayer())
	{
		PC->SetPause(true);
		PC->SetShowMouseCursor(true);
		PC->SetInputMode(FInputModeUIOnly());
	}

	SetUserFocus(GetOwningPlayer());
}

void ULootSelectionWidget::NativeOnDeactivated()
{
	SetVisibility(ESlateVisibility::Collapsed);

	if (APlayerController* PC = GetOwningPlayer())
	{
		PC->SetPause(false);
		PC->SetShowMouseCursor(false);
		PC->SetInputMode(FInputModeGameOnly());
	}

	Super::NativeOnDeactivated();
}

FReply ULootSelectionWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();

	if (Key == EKeys::Gamepad_DPad_Left || Key == EKeys::Left)
	{
		CurrentHighlightIndex = FMath::Max(0, CurrentHighlightIndex - 1);
		OnNavigateSelection(-1);
		OnCardFocused(CurrentHighlightIndex);
		return FReply::Handled();
	}
	if (Key == EKeys::Gamepad_DPad_Right || Key == EKeys::Right)
	{
		CurrentHighlightIndex = FMath::Min(2, CurrentHighlightIndex + 1);
		OnNavigateSelection(1);
		OnCardFocused(CurrentHighlightIndex);
		return FReply::Handled();
	}
	if (Key == EKeys::Gamepad_FaceButton_Bottom || Key == EKeys::Enter)
	{
		SelectRuneLoot(CurrentHighlightIndex);
		return FReply::Handled();
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void ULootSelectionWidget::ConfirmAndTransition()
{
	if (AYogGameMode* GM = Cast<AYogGameMode>(UGameplayStatics::GetGameMode(GetWorld())))
	{
		GM->ConfirmArrangementAndTransition();
	}
}
