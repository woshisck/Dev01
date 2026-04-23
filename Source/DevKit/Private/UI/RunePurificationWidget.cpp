#include "UI/RunePurificationWidget.h"
#include "Character/PlayerCharacterBase.h"
#include "Component/BackpackGridComponent.h"

void URunePurificationWidget::Setup(APlayerCharacterBase* InPlayer)
{
	OwningPlayer   = InPlayer;
	Phase          = 0;
	SelectedRuneGuid = FGuid();
	SelectedCell   = FIntPoint(0, 0);

	if (!InPlayer) return;
	if (UBackpackGridComponent* Grid = InPlayer->FindComponentByClass<UBackpackGridComponent>())
		OnShowRuneList(Grid->GetAllPlacedRunes());
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

		TArray<FIntPoint> Selectable = PR.Rune.Shape.Cells.FilterByPredicate(
			[](const FIntPoint& P) { return P != FIntPoint(0, 0); });
		OnShowCellSelection(RuneGuid, Selectable);
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
	DeactivateWidget();
}

void URunePurificationWidget::CancelPurification()
{
	OnPurificationFinished(false);
	DeactivateWidget();
}
