// Copyright 2021 Mickael Daniel. All Rights Reserved.


#include "UI/GSCUWDebugAbilityQueue.h"

#include "GSCDelegates.h"
#include "Components/CanvasPanel.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Abilities/GSCBlueprintFunctionLibrary.h"
#include "Animation/AnimSequenceBase.h"
#include "Components/GSCAbilityQueueComponent.h"
#include "TimerManager.h"
#include "Abilities/GameplayAbility.h"
#include "GSCLog.h"

void UGSCUWDebugAbilityQueue::SetOwnerActor(AActor* Actor)
{
	Super::SetOwnerActor(Actor);
	OwnerAbilityQueueComponent = UGSCBlueprintFunctionLibrary::GetAbilityQueueComponent(Actor);
}

void UGSCUWDebugAbilityQueue::NativeConstruct()
{
	Super::NativeConstruct();
	
	GSC_UI_LOG(Verbose, TEXT("UGSCUWDebugAbilityQueue Setup Delegates"))
	FGSCDelegates::OnAddAbilityQueueFromMontageRow.AddUObject(this, &UGSCUWDebugAbilityQueue::OnAddAbilityQueueFromMontageRow);
	FGSCDelegates::OnUpdateAllowedAbilities.AddUObject(this, &UGSCUWDebugAbilityQueue::OnUpdateAllowedAbilities);
}

void UGSCUWDebugAbilityQueue::NativeDestruct()
{
	GSC_UI_LOG(Verbose, TEXT("UGSCUWDebugAbilityQueue Clear off delegates"))
	FGSCDelegates::OnAddAbilityQueueFromMontageRow.RemoveAll(this);
	FGSCDelegates::OnUpdateAllowedAbilities.RemoveAll(this);
	
	Super::NativeDestruct();
}

void UGSCUWDebugAbilityQueue::OnAddAbilityQueueFromMontageRow(UAnimSequenceBase* Anim)
{
	GSC_UI_LOG(Verbose, TEXT("UGSCUWDebugAbilityQueue Received OnAddAbilityQueueFromMontageRow: %s"), *GetNameSafe(Anim))
	AddAbilityQueueFromMontageRow(Anim);
}

void UGSCUWDebugAbilityQueue::OnUpdateAllowedAbilities(const TArray<TSubclassOf<UGameplayAbility>> Abilities)
{
	GSC_UI_LOG(Verbose, TEXT("UGSCUWDebugAbilityQueue Received OnUpdateAllowedAbilities: %d"), Abilities.Num())
	UpdateAllowedAbilities(Abilities);
}

void UGSCUWDebugAbilityQueue::UpdateAllowedAbilities(TArray<TSubclassOf<UGameplayAbility>> AllowedAbilities)
{
	if (AllowedAbilitiesBox && AllowedAbilityTemplateText)
	{
		GSC_UI_LOG(Verbose, TEXT("UGSCUWDebugAbilityQueue::UpdateAllowedAbilities() clearing children"))
		AllowedAbilitiesBox->ClearChildren();

		if (OwnerAbilityQueueComponent.IsValid() && OwnerAbilityQueueComponent->IsAllAbilitiesAllowedForAbilityQueue())
		{
			GSC_UI_LOG(Verbose, TEXT("UGSCUWDebugAbilityQueue::UpdateAllowedAbilities() All abilities are allowed"))

			UTextBlock* Text = DuplicateObject(AllowedAbilityTemplateText, this);
			if (Text)
			{
				Text->SetText(FText::FromString("All"));
				AllowedAbilitiesBox->AddChild(Text);
			}
			return;
		}

		for (const TSubclassOf<UGameplayAbility>& Ability : AllowedAbilities)
		{
			if (!Ability)
			{
				continue;
			}

			GSC_UI_LOG(Verbose, TEXT("UGSCUWDebugAbilityQueue::UpdateAllowedAbilities() Adding child for %s"), *Ability->GetName())

			UTextBlock* Text = DuplicateObject(AllowedAbilityTemplateText, this);
			if (Text)
			{
				Text->SetText(FText::FromString(Ability->GetName()));
				AllowedAbilitiesBox->AddChild(Text);
			}
		}
	}
}

void UGSCUWDebugAbilityQueue::AddAbilityQueueFromMontageRow(UAnimSequenceBase* Anim, const bool bStartClearTimer)
{
	if (AbilityQueueFromMontagesBox && AbilityQueueFromMontageTemplateText)
	{
		GSC_UI_LOG(Verbose, TEXT("UGSCUWDebugAbilityQueue::AddAbilityQueueFromMontageRow()"))
		// AllowedAbilitiesBox->ClearChildren();

		UTextBlock* Text = DuplicateObject(AbilityQueueFromMontageTemplateText, this);
		if (Text)
		{
			GSC_UI_LOG(Verbose, TEXT("UGSCUWDebugAbilityQueue::AddAbilityQueueFromMontageRow() Add Child Text"))
			// Text->SetText(FText::FromString("Ability Queue Opened from Montage: " + Anim->GetName()));
			Text->SetText(FText::FromString(Anim->GetName()));
			Text->SetVisibility(ESlateVisibility::HitTestInvisible);
			AbilityQueueFromMontagesBox->AddChild(Text);

			// Ensure initial visibility when rows are added, if it's collapsed on clear (when no rows present)
			if (AbilityQueueFromMontagesPanel)
			{
				AbilityQueueFromMontagesPanel->SetVisibility(ESlateVisibility::HitTestInvisible);
			}

			if (bStartClearTimer)
			{
				StartClearFromMontageRowTimer();
			}
		}
	}
}

void UGSCUWDebugAbilityQueue::StartClearFromMontageRowTimer()
{
	GSC_UI_LOG(Verbose, TEXT("UGSCUWDebugAbilityQueue::StartClearFromMontageRowTimer()"))
	FTimerHandle TimerHandle;
	ClearFromMontageTimerHandles.Add(TimerHandle);
	GetWorld()->GetTimerManager().SetTimer(TimerHandle, this, &UGSCUWDebugAbilityQueue::ClearFromMontageRow, ClearFromMontageDelay, false);
}

void UGSCUWDebugAbilityQueue::NativeTick(const FGeometry& MyGeometry, const float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!OwnerAbilityQueueComponent.IsValid())
	{
		GSC_UI_LOG(Warning, TEXT("UGSCUWDebugAbilityQueue::NativeTick() OwnerAbilityQueueComponent not valid"))
		return;
	}

	if (AbilityQueueEnabledText)
	{
		const bool bAbilityQueueEnabled = OwnerAbilityQueueComponent->bAbilityQueueEnabled;
		AbilityQueueEnabledText->SetText(bAbilityQueueEnabled ? FText::FromString("true") : FText::FromString("false"));
		AbilityQueueEnabledText->SetColorAndOpacity(FSlateColor(bAbilityQueueEnabled ? GreenColor : RedColor));
	}

	if (AbilityQueueOpenedText)
	{
		const bool bAbilityQueueOpened = OwnerAbilityQueueComponent->IsAbilityQueueOpened();
		AbilityQueueOpenedText->SetText(bAbilityQueueOpened ? FText::FromString("true") : FText::FromString("false"));
		AbilityQueueOpenedText->SetColorAndOpacity(FSlateColor(bAbilityQueueOpened ? GreenColor : RedColor));
	}

	if (CurrentQueuedAbilityText)
	{
		const UGameplayAbility* Ability = OwnerAbilityQueueComponent->GetCurrentQueuedAbility();
		CurrentQueuedAbilityText->SetText(Ability ? FText::FromString(Ability->GetName()): FText::FromString("None"));
		CurrentQueuedAbilityText->SetColorAndOpacity(Ability ? GreenColor : WhiteColor);
	}

	if (AllowAllAbilitiesText)
	{
		const bool bAllowAllAbilities = OwnerAbilityQueueComponent->IsAllAbilitiesAllowedForAbilityQueue();
		AllowAllAbilitiesText->SetText(bAllowAllAbilities ? FText::FromString("true"): FText::FromString("false"));
		AllowAllAbilitiesText->SetColorAndOpacity(bAllowAllAbilities ? GreenColor : RedColor);
	}
}

void UGSCUWDebugAbilityQueue::ClearFromMontageRow()
{
	if (AbilityQueueFromMontagesBox)
	{
		AbilityQueueFromMontagesBox->RemoveChildAt(0);

		if (AbilityQueueFromMontagesPanel)
		{
			const int32 ChildrenCount = AbilityQueueFromMontagesBox->GetChildrenCount();
			if (ChildrenCount == 0)
			{
				AbilityQueueFromMontagesPanel->SetVisibility(ESlateVisibility::Collapsed);
			}
		}
	}
}
