#include "LevelFlow/Nodes/LENode_ShowInfoPopup.h"
#include "LevelFlow/LevelEventTrigger.h"
#include "UI/YogHUD.h"
#include "UI/InfoPopupWidget.h"
#include "Data/LevelInfoPopupDA.h"

ULENode_ShowInfoPopup::ULENode_ShowInfoPopup(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("LevelEvent");
#endif
	InputPins  = { FFlowPin(TEXT("In")) };
	OutputPins = { FFlowPin(TEXT("Out")), FFlowPin(TEXT("OnClosed")) };
}

void ULENode_ShowInfoPopup::ExecuteInput(const FName& PinName)
{
	APlayerController* PC = GetPlayerController();
	if (!PC || !PopupDA)
	{
		TriggerOutput(TEXT("Out"), false);
		TriggerOutput(TEXT("OnClosed"), true);
		return;
	}

	AYogHUD* HUD = Cast<AYogHUD>(PC->GetHUD());
	if (!HUD)
	{
		TriggerOutput(TEXT("Out"), false);
		TriggerOutput(TEXT("OnClosed"), true);
		return;
	}

	UInfoPopupWidget* Widget = HUD->GetInfoPopupWidget();
	if (Widget)
	{
		ClosedHandle = Widget->OnClosed.AddLambda([this]()
		{
			TriggerOutput(TEXT("OnClosed"), true);
		});
	}

	HUD->ShowInfoPopup(PopupDA);

	if (Widget)
	{
		if (AActor* TriggerActor = TryGetRootFlowActorOwner())
		{
			if (ALevelEventTrigger* Trigger = Cast<ALevelEventTrigger>(TriggerActor))
			{
				TWeakObjectPtr<UInfoPopupWidget> WeakWidget(Widget);
				Trigger->OnPlayerExited.AddLambda([WeakWidget]()
				{
					if (WeakWidget.IsValid())
						WeakWidget->RequestClose();
				});
			}
		}
	}

	TriggerOutput(TEXT("Out"), false);
}

void ULENode_ShowInfoPopup::Cleanup()
{
	if (APlayerController* PC = GetPlayerController())
		if (AYogHUD* HUD = Cast<AYogHUD>(PC->GetHUD()))
			if (UInfoPopupWidget* Widget = HUD->GetInfoPopupWidget())
				Widget->OnClosed.Remove(ClosedHandle);

	ClosedHandle.Reset();
	Super::Cleanup();
}
