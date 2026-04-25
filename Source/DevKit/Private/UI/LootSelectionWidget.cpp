// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/LootSelectionWidget.h"
#include "GameModes/YogGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "Character/YogPlayerControllerBase.h"
#include "Input/CommonUIInputTypes.h"
#include "UI/YogHUD.h"

void ULootSelectionWidget::NativeConstruct()
{
	Super::NativeConstruct();

	SetVisibility(ESlateVisibility::Collapsed);
	bIsFocusable = true;


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

void ULootSelectionWidget::ShowLootUI(const TArray<FLootOption>& Options)
{

	CurrentLootOptions = Options;
	OnLootOptionsReady(Options);

	SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	if (APlayerController* PC = GetOwningPlayer())
	{
		if (AYogHUD* HUD = Cast<AYogHUD>(PC->GetHUD()))
			HUD->BeginPauseEffect();
		PC->SetPause(true);
		PC->SetShowMouseCursor(true);
		FInputModeUIOnly InputMode;
		InputMode.SetWidgetToFocus(GetCachedWidget());
		PC->SetInputMode(InputMode);
	}

	SetUserFocus(GetOwningPlayer());
	CurrentHighlightIndex = 0;
	bStickNavHeld = false;
	OnCardFocused(0);
	UE_LOG(LogTemp, Log, TEXT("[LootSelection] ShowLootUI: HasFocus=%d, CachedWidget=%d"),
		HasUserFocus(GetOwningPlayer()), GetCachedWidget().IsValid());
}

void ULootSelectionWidget::HandleLootGenerated(const TArray<FLootOption>& LootOptions)
{
	ShowLootUI(LootOptions);
}

void ULootSelectionWidget::HandlePhaseChanged(ELevelPhase NewPhase)
{
	OnLevelPhaseChanged(NewPhase);
}

void ULootSelectionWidget::SelectRuneLoot(int32 Index)
{
	if (AYogGameMode* GM = Cast<AYogGameMode>(UGameplayStatics::GetGameMode(GetWorld())))
		GM->SelectLoot(Index);

	// 直接收起，不调 DeactivateWidget（会触发 CommonUI destroy）
	SetVisibility(ESlateVisibility::Collapsed);

	if (APlayerController* PC = GetOwningPlayer())
	{
		// 先恢复游戏状态，再开背包（背包会自己设 UIOnly 输入模式）
		PC->SetPause(false);
		PC->SetShowMouseCursor(false);
		PC->SetInputMode(FInputModeGameOnly());
		if (AYogHUD* HUD = Cast<AYogHUD>(PC->GetHUD()))
		{
			HUD->EndPauseEffect();
			HUD->OpenBackpack();
		}
	}
}

// ============================================================
//  CommonUI 生命周期（仅供 Stack 场景保留，手动放置路径不走这里）
// ============================================================

TOptional<FUIInputConfig> ULootSelectionWidget::GetDesiredInputConfig() const
{
	// 不让 CommonUI 管理此 Widget 的输入，避免 SetInputMode 触发 DeactivateWidget
	return TOptional<FUIInputConfig>();
}

void ULootSelectionWidget::NativeOnActivated()
{
	Super::NativeOnActivated();
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	if (APlayerController* PC = GetOwningPlayer())
	{
		if (AYogHUD* HUD = Cast<AYogHUD>(PC->GetHUD()))
			HUD->BeginPauseEffect();
		PC->SetPause(true);
		PC->SetShowMouseCursor(true);
		FInputModeUIOnly InputMode;
		InputMode.SetWidgetToFocus(GetCachedWidget());
		PC->SetInputMode(InputMode);
	}

	SetUserFocus(GetOwningPlayer());
}

void ULootSelectionWidget::NativeOnFocusLost(const FFocusEvent& InFocusEvent)
{
	// 不调 Super：UUserWidget::NativeOnFocusLost 会触发 BP_OnFocusLost，
	// WBP 里可能有 Event On Focus Lost → RemoveFromParent 导致 Widget 被销毁
}

void ULootSelectionWidget::NativeOnDeactivated()
{

	SetVisibility(ESlateVisibility::Collapsed);

	if (APlayerController* PC = GetOwningPlayer())
	{
		PC->SetPause(false);
		PC->SetShowMouseCursor(false);
		PC->SetInputMode(FInputModeGameOnly());
		if (AYogHUD* HUD = Cast<AYogHUD>(PC->GetHUD()))
			HUD->EndPauseEffect();
	}

	// 不调 Super：CommonUI 的 NativeOnDeactivated 会检查 bDestroyOnDeactivation → RemoveFromParent
	// 我们手动管理生命周期，Widget 保持在 Viewport 中只改可见性
}

FReply ULootSelectionWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	UE_LOG(LogTemp, Log, TEXT("[LootSelection] NativeOnKeyDown: %s"), *Key.ToString());

	if (Key == EKeys::Gamepad_DPad_Left || Key == EKeys::Gamepad_LeftShoulder || Key == EKeys::Left)
	{
		CurrentHighlightIndex = FMath::Max(0, CurrentHighlightIndex - 1);
		UE_LOG(LogTemp, Log, TEXT("[LootSelection] Navigate Left → Index=%d"), CurrentHighlightIndex);
		OnNavigateSelection(-1);
		OnCardFocused(CurrentHighlightIndex);
		return FReply::Handled();
	}
	if (Key == EKeys::Gamepad_DPad_Right || Key == EKeys::Gamepad_RightShoulder || Key == EKeys::Right)
	{
		CurrentHighlightIndex = FMath::Min(2, CurrentHighlightIndex + 1);
		UE_LOG(LogTemp, Log, TEXT("[LootSelection] Navigate Right → Index=%d"), CurrentHighlightIndex);
		OnNavigateSelection(1);
		OnCardFocused(CurrentHighlightIndex);
		return FReply::Handled();
	}
	if (Key == EKeys::Gamepad_FaceButton_Bottom || Key == EKeys::Enter)
	{
		UE_LOG(LogTemp, Log, TEXT("[LootSelection] Confirm → SelectRuneLoot(%d)"), CurrentHighlightIndex);
		SelectRuneLoot(CurrentHighlightIndex);
		return FReply::Handled();
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

FReply ULootSelectionWidget::NativeOnAnalogValueChanged(const FGeometry& InGeometry, const FAnalogInputEvent& InAnalogInputEvent)
{
	if (GetVisibility() == ESlateVisibility::Collapsed)
		return Super::NativeOnAnalogValueChanged(InGeometry, InAnalogInputEvent);

	const FKey Key = InAnalogInputEvent.GetKey();
	const float Value = InAnalogInputEvent.GetAnalogValue();

	if (Key == EKeys::Gamepad_LeftX)
	{
		if (FMath::Abs(Value) > 0.5f && !bStickNavHeld)
		{
			bStickNavHeld = true;
			const int32 Delta = (Value < 0.f) ? -1 : 1;
			CurrentHighlightIndex = FMath::Clamp(CurrentHighlightIndex + Delta, 0, 2);
			UE_LOG(LogTemp, Log, TEXT("[LootSelection] LeftStick X=%.2f → Navigate %s → Index=%d"),
				Value, Delta < 0 ? TEXT("Left") : TEXT("Right"), CurrentHighlightIndex);
			OnNavigateSelection(Delta);
			OnCardFocused(CurrentHighlightIndex);
			return FReply::Handled();
		}
		if (FMath::Abs(Value) < 0.3f)
			bStickNavHeld = false;
	}

	return Super::NativeOnAnalogValueChanged(InGeometry, InAnalogInputEvent);
}

void ULootSelectionWidget::ConfirmAndTransition()
{
	if (AYogGameMode* GM = Cast<AYogGameMode>(UGameplayStatics::GetGameMode(GetWorld())))
		GM->ConfirmArrangementAndTransition();
}
