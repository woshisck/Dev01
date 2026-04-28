#include "UI/SacrificeGraceOptionWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Input/CommonUIInputTypes.h"
#include "Data/SacrificeGraceDA.h"
#include "Character/PlayerCharacterBase.h"
#include "Map/SacrificeGracePickup.h"

void USacrificeGraceOptionWidget::Setup(USacrificeGraceDA* InDA, APlayerCharacterBase* InPlayer, ASacrificeGracePickup* InPickup)
{
	SacrificeDA  = InDA;
	OwningPlayer = InPlayer;
	SourcePickup = InPickup;

	if (TitleText && InDA)
		TitleText->SetText(InDA->DisplayName);
	if (DescriptionText && InDA)
		DescriptionText->SetText(InDA->Description);

	OnSetup(InDA);
}

void USacrificeGraceOptionWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (BtnYes) BtnYes->OnClicked.AddDynamic(this, &USacrificeGraceOptionWidget::OnYesClicked);
	if (BtnNo)  BtnNo->OnClicked.AddDynamic(this, &USacrificeGraceOptionWidget::OnNoClicked);
}

void USacrificeGraceOptionWidget::NativeOnActivated()
{
	Super::NativeOnActivated();
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}

void USacrificeGraceOptionWidget::NativeOnDeactivated()
{
	Super::NativeOnDeactivated();
	SetVisibility(ESlateVisibility::Collapsed);
}

TOptional<FUIInputConfig> USacrificeGraceOptionWidget::GetDesiredInputConfig() const
{
	return FUIInputConfig(ECommonInputMode::Menu, EMouseCaptureMode::NoCapture);
}

UWidget* USacrificeGraceOptionWidget::NativeGetDesiredFocusTarget() const
{
	return BtnYes;
}

void USacrificeGraceOptionWidget::OnYesClicked()
{
	if (APlayerCharacterBase* Player = OwningPlayer.Get())
		Player->AcquireSacrificeGrace(SacrificeDA);

	if (ASacrificeGracePickup* Pickup = SourcePickup.Get())
		Pickup->ConsumeAndDestroy();

	DeactivateWidget();
}

void USacrificeGraceOptionWidget::OnNoClicked()
{
	if (ASacrificeGracePickup* Pickup = SourcePickup.Get())
		Pickup->ResetForSkip(OwningPlayer.Get());

	DeactivateWidget();
}
