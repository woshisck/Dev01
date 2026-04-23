#include "UI/AltarMenuWidget.h"
#include "UI/RunePurificationWidget.h"
#include "UI/SacrificeSelectionWidget.h"
#include "Character/PlayerCharacterBase.h"

void UAltarMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();
}

void UAltarMenuWidget::SetupAltar(UAltarDataAsset* InData, APlayerCharacterBase* InPlayer)
{
	AltarData = InData;
	OwningPlayer = InPlayer;

	if (PurificationWidgetClass && !PurificationWidget)
	{
		PurificationWidget = CreateWidget<URunePurificationWidget>(GetOwningPlayer(), PurificationWidgetClass);
		if (PurificationWidget)
			PurificationWidget->AddToViewport(11);
	}
	if (SacrificeWidgetClass && !SacrificeWidget)
	{
		SacrificeWidget = CreateWidget<USacrificeSelectionWidget>(GetOwningPlayer(), SacrificeWidgetClass);
		if (SacrificeWidget)
			SacrificeWidget->AddToViewport(11);
	}
}

void UAltarMenuWidget::OpenPurification()
{
	if (!PurificationWidget || !OwningPlayer.IsValid()) return;
	DeactivateWidget();
	PurificationWidget->Setup(OwningPlayer.Get());
	PurificationWidget->ActivateWidget();
}

void UAltarMenuWidget::OpenSacrifice()
{
	if (!SacrificeWidget || !AltarData || !OwningPlayer.IsValid()) return;
	DeactivateWidget();
	SacrificeWidget->Setup(AltarData, OwningPlayer.Get());
	SacrificeWidget->ActivateWidget();
}

void UAltarMenuWidget::CloseMenu()
{
	DeactivateWidget();
}
