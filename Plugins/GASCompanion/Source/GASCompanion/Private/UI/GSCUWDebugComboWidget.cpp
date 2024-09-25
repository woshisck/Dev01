// Copyright 2021 Mickael Daniel. All Rights Reserved.


#include "UI/GSCUWDebugComboWidget.h"

#include "Abilities/GSCBlueprintFunctionLibrary.h"
#include "Components/GSCComboManagerComponent.h"
#include "Components/TextBlock.h"

void UGSCUWDebugComboWidget::SetOwnerActor(AActor* Actor)
{
	Super::SetOwnerActor(Actor);
	OwnerComboManagerComponent = UGSCBlueprintFunctionLibrary::GetComboManagerComponent(Actor);
}

void UGSCUWDebugComboWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!OwnerComboManagerComponent.IsValid())
	{
		return;
	}

	if (ComboWindowOpenedText)
	{
		ComboWindowOpenedText->SetText(FText::FromString(OwnerComboManagerComponent->bComboWindowOpened ? TEXT("true") : TEXT("false")));
		ComboWindowOpenedText->SetColorAndOpacity(FSlateColor(OwnerComboManagerComponent->bComboWindowOpened ? GreenColor : RedColor));
	}

	if (ShouldTriggerComboText)
	{
		ShouldTriggerComboText->SetText(FText::FromString(OwnerComboManagerComponent->bShouldTriggerCombo ? TEXT("true") : TEXT("false")));
		ShouldTriggerComboText->SetColorAndOpacity(FSlateColor(OwnerComboManagerComponent->bShouldTriggerCombo ? GreenColor : RedColor));
	}

	if (RequestTriggerComboText)
	{
		RequestTriggerComboText->SetText(FText::FromString(OwnerComboManagerComponent->bRequestTriggerCombo ? TEXT("true") : TEXT("false")));
		RequestTriggerComboText->SetColorAndOpacity(FSlateColor(OwnerComboManagerComponent->bRequestTriggerCombo ? GreenColor : RedColor));
	}

	if (NextComboAbilityActivatedText)
	{
		NextComboAbilityActivatedText->SetText(FText::FromString(OwnerComboManagerComponent->bNextComboAbilityActivated ? TEXT("true") : TEXT("false")));
		NextComboAbilityActivatedText->SetColorAndOpacity(FSlateColor(OwnerComboManagerComponent->bNextComboAbilityActivated ? GreenColor : RedColor));
	}

	if (ComboIndexText)
	{
		ComboIndexText->SetText(FText::FromString(FString::FromInt(OwnerComboManagerComponent->ComboIndex)));
		ComboIndexText->SetColorAndOpacity(FSlateColor(OwnerComboManagerComponent->ComboIndex == 0 ? WhiteColor : GreenColor));
	}
}
