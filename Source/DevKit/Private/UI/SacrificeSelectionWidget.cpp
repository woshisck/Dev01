#include "UI/SacrificeSelectionWidget.h"
#include "Character/PlayerCharacterBase.h"
#include "Algo/RandomShuffle.h"
#include "UI/YogHUD.h"
#include "Input/CommonUIInputTypes.h"
#include "InputCoreTypes.h"

TOptional<FUIInputConfig> USacrificeSelectionWidget::GetDesiredInputConfig() const
{
	return FUIInputConfig(ECommonInputMode::Menu, EMouseCaptureMode::NoCapture);
}

void USacrificeSelectionWidget::NativeOnActivated()
{
	Super::NativeOnActivated();
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	SetIsFocusable(true);

	if (APlayerController* PC = GetOwningPlayer())
	{
		if (AYogHUD* HUD = Cast<AYogHUD>(PC->GetHUD()))
			HUD->BeginPauseEffect();

		PC->SetShowMouseCursor(true);
		FInputModeUIOnly InputMode;
		InputMode.SetWidgetToFocus(GetCachedWidget());
		PC->SetInputMode(InputMode);
	}
	SetUserFocus(GetOwningPlayer());
}

void USacrificeSelectionWidget::NativeOnDeactivated()
{
	SetVisibility(ESlateVisibility::Collapsed);
	if (APlayerController* PC = GetOwningPlayer())
	{
		PC->SetShowMouseCursor(false);
		PC->SetInputMode(FInputModeGameOnly());
		if (AYogHUD* HUD = Cast<AYogHUD>(PC->GetHUD()))
			HUD->EndPauseEffect();
	}
	Super::NativeOnDeactivated();
}

FReply USacrificeSelectionWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	if (Key == EKeys::Escape ||
		Key == EKeys::Gamepad_FaceButton_Right ||
		Key == EKeys::Gamepad_Special_Right)
	{
		CancelSacrifice();
		return FReply::Handled();
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void USacrificeSelectionWidget::Setup(UAltarDataAsset* InData, APlayerCharacterBase* InPlayer)
{
	AltarData           = InData;
	OwningPlayer        = InPlayer;
	Phase               = 0;
	SelectedOptionIndex = -1;
	CurrentOptions.Reset();

	if (!InData || InData->SacrificeRunePool.IsEmpty()) return;

	// 随机打乱池，取前三个
	TArray<int32> Indices;
	for (int32 i = 0; i < InData->SacrificeRunePool.Num(); i++) Indices.Add(i);
	Algo::RandomShuffle(Indices);
	for (int32 i = 0; i < FMath::Min(3, Indices.Num()); i++)
		CurrentOptions.Add(InData->SacrificeRunePool[Indices[i]]);

	OnShowSacrificeOptions(CurrentOptions);
}

void USacrificeSelectionWidget::SelectSacrificeOption(int32 OptionIndex)
{
	if (Phase != 0 || !CurrentOptions.IsValidIndex(OptionIndex)) return;
	SelectedOptionIndex = OptionIndex;
	Phase = 1;
	OnShowCostConfirmation(CurrentOptions[OptionIndex]);
}

void USacrificeSelectionWidget::ConfirmSacrifice()
{
	if (Phase != 1 || !CurrentOptions.IsValidIndex(SelectedOptionIndex)) return;
	if (!OwningPlayer.IsValid()) return;

	const FAltarSacrificeEntry& Entry = CurrentOptions[SelectedOptionIndex];
	if (Entry.GrantedRune)
		OwningPlayer->AddRuneToInventory(Entry.GrantedRune->CreateInstance());
	// 代价逻辑由 GrantedRune 的 FlowAsset 在激活时执行

	OnSacrificeFinished(true);
	DeactivateWidget();
}

void USacrificeSelectionWidget::CancelSacrifice()
{
	if (Phase == 1)
	{
		// Phase 1 取消：返回 Phase 0 重新选
		Phase = 0;
		SelectedOptionIndex = -1;
		OnShowSacrificeOptions(CurrentOptions);
	}
	else
	{
		// Phase 0 取消：关闭整个面板
		OnSacrificeFinished(false);
		DeactivateWidget();
	}
}
