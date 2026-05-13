#include "UI/RunePurificationWidget.h"
#include "Character/PlayerCharacterBase.h"
#include "Component/BackpackGridComponent.h"
#include "UI/YogHUD.h"
#include "Input/CommonUIInputTypes.h"
#include "InputCoreTypes.h"
#include "UI/YogInputKeyUtils.h"
#include "UI/YogUIManagerSubsystem.h"

TOptional<FUIInputConfig> URunePurificationWidget::GetDesiredInputConfig() const
{
	return FUIInputConfig(ECommonInputMode::Menu, EMouseCaptureMode::NoCapture);
}

void URunePurificationWidget::NativeOnActivated()
{
	Super::NativeOnActivated();
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	SetIsFocusable(true);

	// SetUserFocus(player) was targeting the wrong root — leave focus routing to GetDesiredFocusTarget.
}

void URunePurificationWidget::NativeOnDeactivated()
{
	SetVisibility(ESlateVisibility::Collapsed);
	Super::NativeOnDeactivated();
}

FReply URunePurificationWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	if (YogInputKeys::IsBackKey(Key) || YogInputKeys::IsMenuKey(Key))
	{
		CancelPurification();
		return FReply::Handled();
	}

	const int32 NavDirection = YogInputKeys::GetVerticalNavigationDirection(Key) != 0
		? YogInputKeys::GetVerticalNavigationDirection(Key)
		: YogInputKeys::GetHorizontalNavigationDirection(Key);
	if (NavDirection != 0)
	{
		MoveFocus(NavDirection);
		return FReply::Handled();
	}

	if (YogInputKeys::IsAcceptKey(Key))
	{
		ActivateFocusedEntry();
		return FReply::Handled();
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

FReply URunePurificationWidget::NativeOnAnalogValueChanged(const FGeometry& InGeometry, const FAnalogInputEvent& InAnalogInputEvent)
{
	const FKey Key = InAnalogInputEvent.GetKey();
	const float Value = InAnalogInputEvent.GetAnalogValue();
	if ((Key == EKeys::Gamepad_LeftY || Key == EKeys::Gamepad_LeftX) && FMath::Abs(Value) >= 0.65f)
	{
		const float Now = GetWorld() ? GetWorld()->GetRealTimeSeconds() : 0.f;
		if (Now - LastAnalogNavigationTime >= 0.18f)
		{
			LastAnalogNavigationTime = Now;
			const int32 Direction = (Key == EKeys::Gamepad_LeftY)
				? (Value > 0.f ? -1 : 1)
				: (Value < 0.f ? -1 : 1);
			MoveFocus(Direction);
			return FReply::Handled();
		}
	}

	return Super::NativeOnAnalogValueChanged(InGeometry, InAnalogInputEvent);
}

void URunePurificationWidget::Setup(APlayerCharacterBase* InPlayer)
{
	OwningPlayer   = InPlayer;
	Phase          = 0;
	SelectedRuneGuid = FGuid();
	SelectedCell   = FIntPoint(0, 0);
	CachedPlacedRunes.Reset();
	CachedSelectableCells.Reset();
	FocusedIndex = 0;

	if (!InPlayer) return;
	if (UBackpackGridComponent* Grid = InPlayer->FindComponentByClass<UBackpackGridComponent>())
	{
		CachedPlacedRunes = Grid->GetAllPlacedRunes();
		OnShowRuneList(CachedPlacedRunes);
		OnNativeFocusIndexChanged(Phase, FocusedIndex);
	}
}

void URunePurificationWidget::SelectRune(FGuid RuneGuid)
{
	if (Phase != 0 || !OwningPlayer.IsValid()) return;
	UBackpackGridComponent* Grid = OwningPlayer->FindComponentByClass<UBackpackGridComponent>();
	if (!Grid) return;

	for (const FPlacedRune& PR : Grid->GetAllPlacedRunes())
	{
		if (PR.Rune.RuneGuid != RuneGuid) continue;
		SelectedRuneGuid = RuneGuid;
		Phase = 1;
		FocusedIndex = 0;

		CachedSelectableCells = PR.Rune.Shape.Cells.FilterByPredicate(
			[](const FIntPoint& P) { return P != FIntPoint(0, 0); });
		OnShowCellSelection(RuneGuid, CachedSelectableCells);
		OnNativeFocusIndexChanged(Phase, FocusedIndex);
		return;
	}
}

void URunePurificationWidget::SelectCell(FIntPoint LocalCell)
{
	if (Phase != 1 || LocalCell == FIntPoint(0, 0)) return;
	SelectedCell = LocalCell;
}

void URunePurificationWidget::ConfirmPurification()
{
	if (Phase != 1 || SelectedCell == FIntPoint(0, 0)) return;
	if (!OwningPlayer.IsValid()) return;

	UBackpackGridComponent* Grid = OwningPlayer->FindComponentByClass<UBackpackGridComponent>();
	bool bOk = Grid && Grid->TryRemoveRuneCell(SelectedRuneGuid, SelectedCell);
	OnPurificationFinished(bOk);
	if (!UYogUIManagerSubsystem::PopManagedScreen(this, EYogUIScreenId::RunePurification))
	{
		DeactivateWidget();
	}
}

void URunePurificationWidget::CancelPurification()
{
	if (Phase == 1)
	{
		Phase = 0;
		SelectedRuneGuid = FGuid();
		SelectedCell = FIntPoint(0, 0);
		CachedSelectableCells.Reset();
		FocusedIndex = 0;
		OnShowRuneList(CachedPlacedRunes);
		OnNativeFocusIndexChanged(Phase, FocusedIndex);
		return;
	}

	OnPurificationFinished(false);
	if (!UYogUIManagerSubsystem::PopManagedScreen(this, EYogUIScreenId::RunePurification))
	{
		DeactivateWidget();
	}
}

void URunePurificationWidget::MoveFocus(int32 Direction)
{
	const int32 Count = GetCurrentFocusCount();
	if (Count <= 0)
	{
		return;
	}

	FocusedIndex = (FocusedIndex + Direction) % Count;
	if (FocusedIndex < 0)
	{
		FocusedIndex += Count;
	}
	OnNativeFocusIndexChanged(Phase, FocusedIndex);
}

void URunePurificationWidget::ActivateFocusedEntry()
{
	if (Phase == 0)
	{
		if (CachedPlacedRunes.IsValidIndex(FocusedIndex))
		{
			SelectRune(CachedPlacedRunes[FocusedIndex].Rune.RuneGuid);
		}
		return;
	}

	if (Phase == 1)
	{
		if (SelectedCell == FIntPoint(0, 0))
		{
			if (CachedSelectableCells.IsValidIndex(FocusedIndex))
			{
				SelectCell(CachedSelectableCells[FocusedIndex]);
				OnNativeFocusIndexChanged(Phase, FocusedIndex);
			}
			return;
		}

		ConfirmPurification();
	}
}

int32 URunePurificationWidget::GetCurrentFocusCount() const
{
	return Phase == 0 ? CachedPlacedRunes.Num() : CachedSelectableCells.Num();
}
