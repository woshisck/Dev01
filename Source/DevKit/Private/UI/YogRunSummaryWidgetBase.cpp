#include "UI/YogRunSummaryWidgetBase.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Kismet/GameplayStatics.h"

void UYogRunSummaryWidgetBase::NativeConstruct()
{
	Super::NativeConstruct();
	if (WidgetTree)
	{
		UVerticalBox* RootBox = Cast<UVerticalBox>(WidgetTree->RootWidget);
		if (!RootBox)
		{
			RootBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Root"));
			WidgetTree->RootWidget = RootBox;
		}
		if (!TxtFloorReached)
		{
			TxtFloorReached = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TxtFloorReached"));
			RootBox->AddChildToVerticalBox(TxtFloorReached);
		}
		if (!TxtEnemiesKilled)
		{
			TxtEnemiesKilled = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TxtEnemiesKilled"));
			RootBox->AddChildToVerticalBox(TxtEnemiesKilled);
		}
		if (!BtnReturnToHub)
		{
			BtnReturnToHub = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("BtnReturnToHub"));
			UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TxtReturnToHubLabel"));
			Label->SetText(NSLOCTEXT("RunSummary", "ReturnToHub", "Return to Hub"));
			BtnReturnToHub->AddChild(Label);
			RootBox->AddChildToVerticalBox(BtnReturnToHub);
		}
	}

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
