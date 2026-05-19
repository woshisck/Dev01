#include "UI/YogRunSummaryWidgetBase.h"
#include "Components/Button.h"
#include "Kismet/GameplayStatics.h"

void UYogRunSummaryWidgetBase::NativeConstruct()
{
	Super::NativeConstruct();
	if (BtnReturnToHub)
	{
		BtnReturnToHub->OnClicked.AddDynamic(this, &UYogRunSummaryWidgetBase::HandleReturnToHubClicked);
	}
}

void UYogRunSummaryWidgetBase::ShowSummary(const FRunSummaryData& InSummary)
{
	CurrentSummary = InSummary;

	if (TxtFloorReached)
	{
		TxtFloorReached->SetText(FText::AsNumber(InSummary.FloorReached));
	}
	if (TxtEnemiesKilled)
	{
		TxtEnemiesKilled->SetText(FText::AsNumber(InSummary.EnemiesKilled));
	}

	BP_OnSummaryReceived(InSummary);
	ActivateWidget();
}

void UYogRunSummaryWidgetBase::HandleReturnToHubClicked()
{
	OnReturnToHubRequested.Broadcast();
	DeactivateWidget();
	if (!HubLevelName.IsNone())
	{
		UGameplayStatics::OpenLevel(this, HubLevelName);
	}
}
