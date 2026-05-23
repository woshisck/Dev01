#include "UI/YogSlotSelectWidgetBase.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "SaveGame/YogSaveSubsystem.h"
#include "System/YogGameInstanceBase.h"

namespace
{
FString FormatDuration(int32 TotalSeconds)
{
	TotalSeconds = FMath::Max(0, TotalSeconds);
	const int32 Hours = TotalSeconds / 3600;
	const int32 Minutes = (TotalSeconds / 60) % 60;
	const int32 Seconds = TotalSeconds % 60;
	return FString::Printf(TEXT("%02d:%02d:%02d"), Hours, Minutes, Seconds);
}
}

FText UYogSlotSelectWidgetBase::BuildPreviewSummary(const FSlotPreviewData& Preview)
{
	if (!Preview.bHasData)
	{
		return FText::FromString(TEXT("Empty Slot"));
	}

	FString RunState = TEXT("No Pending Run");
	if (Preview.bHasPendingRun)
	{
		RunState = TEXT("Continue Available");
	}
	else if (Preview.bFirstRunTutorialActive)
	{
		RunState = TEXT("First Run Tutorial");
	}
	const FString LastPlayed = Preview.LastPlayTime.GetTicks() > 0
		? Preview.LastPlayTime.ToString(TEXT("%Y-%m-%d %H:%M"))
		: FString(TEXT("Never"));

	return FText::FromString(FString::Printf(
		TEXT("Floor %d\n%s\nPlayed %s\nLast %s"),
		Preview.HighestFloor,
		*RunState,
		*FormatDuration(Preview.TotalPlayTimeSeconds),
		*LastPlayed));
}

void UYogSlotSelectWidgetBase::NativeConstruct()
{
	Super::NativeConstruct();
	BindButtons();
	RequestAllPreviews();
}

void UYogSlotSelectWidgetBase::NativeOnActivated()
{
	Super::NativeOnActivated();
	RequestAllPreviews();
}

UWidget* UYogSlotSelectWidgetBase::NativeGetDesiredFocusTarget() const
{
	if (BtnContinue_0 && BtnContinue_0->GetIsEnabled())
	{
		return BtnContinue_0;
	}
	return BtnNewGame_0.Get();
}

void UYogSlotSelectWidgetBase::RequestAllPreviews()
{
	UYogSaveSubsystem* SaveSys = GetSaveSubsystem();
	if (!SaveSys)
	{
		for (int32 SlotIndex = 0; SlotIndex < 3; ++SlotIndex)
		{
			UpdateSlotWidgets(SlotIndex, FSlotPreviewData{});
		}
		return;
	}

	FOnSlotPreviewReady Preview0;
	Preview0.BindDynamic(this, &UYogSlotSelectWidgetBase::ApplyPreview_0);
	SaveSys->RequestSlotPreview(0, Preview0);

	FOnSlotPreviewReady Preview1;
	Preview1.BindDynamic(this, &UYogSlotSelectWidgetBase::ApplyPreview_1);
	SaveSys->RequestSlotPreview(1, Preview1);

	FOnSlotPreviewReady Preview2;
	Preview2.BindDynamic(this, &UYogSlotSelectWidgetBase::ApplyPreview_2);
	SaveSys->RequestSlotPreview(2, Preview2);
}

UYogSaveSubsystem* UYogSlotSelectWidgetBase::GetSaveSubsystem() const
{
	UGameInstance* GameInstance = GetGameInstance();
	return GameInstance ? GameInstance->GetSubsystem<UYogSaveSubsystem>() : nullptr;
}

void UYogSlotSelectWidgetBase::BindButtons()
{
	if (BtnContinue_0)
	{
		BtnContinue_0->OnClicked.RemoveDynamic(this, &UYogSlotSelectWidgetBase::HandleContinueSlot0);
		BtnContinue_0->OnClicked.AddDynamic(this, &UYogSlotSelectWidgetBase::HandleContinueSlot0);
	}
	if (BtnContinue_1)
	{
		BtnContinue_1->OnClicked.RemoveDynamic(this, &UYogSlotSelectWidgetBase::HandleContinueSlot1);
		BtnContinue_1->OnClicked.AddDynamic(this, &UYogSlotSelectWidgetBase::HandleContinueSlot1);
	}
	if (BtnContinue_2)
	{
		BtnContinue_2->OnClicked.RemoveDynamic(this, &UYogSlotSelectWidgetBase::HandleContinueSlot2);
		BtnContinue_2->OnClicked.AddDynamic(this, &UYogSlotSelectWidgetBase::HandleContinueSlot2);
	}
	if (BtnNewGame_0)
	{
		BtnNewGame_0->OnClicked.RemoveDynamic(this, &UYogSlotSelectWidgetBase::HandleNewGameSlot0);
		BtnNewGame_0->OnClicked.AddDynamic(this, &UYogSlotSelectWidgetBase::HandleNewGameSlot0);
	}
	if (BtnNewGame_1)
	{
		BtnNewGame_1->OnClicked.RemoveDynamic(this, &UYogSlotSelectWidgetBase::HandleNewGameSlot1);
		BtnNewGame_1->OnClicked.AddDynamic(this, &UYogSlotSelectWidgetBase::HandleNewGameSlot1);
	}
	if (BtnNewGame_2)
	{
		BtnNewGame_2->OnClicked.RemoveDynamic(this, &UYogSlotSelectWidgetBase::HandleNewGameSlot2);
		BtnNewGame_2->OnClicked.AddDynamic(this, &UYogSlotSelectWidgetBase::HandleNewGameSlot2);
	}
	if (BtnDelete_0)
	{
		BtnDelete_0->OnClicked.RemoveDynamic(this, &UYogSlotSelectWidgetBase::HandleDeleteSlot0);
		BtnDelete_0->OnClicked.AddDynamic(this, &UYogSlotSelectWidgetBase::HandleDeleteSlot0);
	}
	if (BtnDelete_1)
	{
		BtnDelete_1->OnClicked.RemoveDynamic(this, &UYogSlotSelectWidgetBase::HandleDeleteSlot1);
		BtnDelete_1->OnClicked.AddDynamic(this, &UYogSlotSelectWidgetBase::HandleDeleteSlot1);
	}
	if (BtnDelete_2)
	{
		BtnDelete_2->OnClicked.RemoveDynamic(this, &UYogSlotSelectWidgetBase::HandleDeleteSlot2);
		BtnDelete_2->OnClicked.AddDynamic(this, &UYogSlotSelectWidgetBase::HandleDeleteSlot2);
	}
}

void UYogSlotSelectWidgetBase::SelectSlot(int32 SlotIndex)
{
	if (UYogSaveSubsystem* SaveSys = GetSaveSubsystem())
	{
		SaveSys->SelectSlot(SlotIndex);
	}
}

void UYogSlotSelectWidgetBase::ContinueSlot(int32 SlotIndex)
{
	SelectSlot(SlotIndex);
	if (UYogGameInstanceBase* GI = Cast<UYogGameInstanceBase>(GetGameInstance()))
	{
		GI->ContinueRunFromFrontend();
	}
}

void UYogSlotSelectWidgetBase::NewGameSlot(int32 SlotIndex)
{
	if (UYogSaveSubsystem* SaveSys = GetSaveSubsystem())
	{
		SaveSys->ResetSlotForNewGame(SlotIndex);
	}

	if (UYogGameInstanceBase* GI = Cast<UYogGameInstanceBase>(GetGameInstance()))
	{
		GI->StartNewRunFromFrontend();
	}
}

void UYogSlotSelectWidgetBase::DeleteSlot(int32 SlotIndex)
{
	if (UYogSaveSubsystem* SaveSys = GetSaveSubsystem())
	{
		SaveSys->DeleteSlot(SlotIndex);
	}

	CachedPreviews[SlotIndex] = FSlotPreviewData{};
	UpdateSlotWidgets(SlotIndex, CachedPreviews[SlotIndex]);
	RequestAllPreviews();
}

void UYogSlotSelectWidgetBase::ApplyPreview(int32 SlotIndex, const FSlotPreviewData& Preview)
{
	if (SlotIndex < 0 || SlotIndex >= 3)
	{
		return;
	}

	CachedPreviews[SlotIndex] = Preview;
	UpdateSlotWidgets(SlotIndex, Preview);
}

void UYogSlotSelectWidgetBase::UpdateSlotWidgets(int32 SlotIndex, const FSlotPreviewData& Preview)
{
	if (UTextBlock* TitleText = GetTitleText(SlotIndex))
	{
		TitleText->SetText(FText::FromString(FString::Printf(TEXT("Slot %d"), SlotIndex + 1)));
	}

	if (UTextBlock* PreviewText = GetPreviewText(SlotIndex))
	{
		PreviewText->SetText(BuildPreviewSummary(Preview));
	}

	if (UButton* ContinueButton = GetContinueButton(SlotIndex))
	{
		ContinueButton->SetIsEnabled(Preview.bHasPendingRun || Preview.bFirstRunTutorialActive);
	}

	if (UButton* DeleteButton = GetDeleteButton(SlotIndex))
	{
		DeleteButton->SetIsEnabled(Preview.bHasData);
	}
}

UTextBlock* UYogSlotSelectWidgetBase::GetTitleText(int32 SlotIndex) const
{
	switch (SlotIndex)
	{
	case 0: return SlotTitleText_0.Get();
	case 1: return SlotTitleText_1.Get();
	case 2: return SlotTitleText_2.Get();
	default: return nullptr;
	}
}

UTextBlock* UYogSlotSelectWidgetBase::GetPreviewText(int32 SlotIndex) const
{
	switch (SlotIndex)
	{
	case 0: return SlotPreviewText_0.Get();
	case 1: return SlotPreviewText_1.Get();
	case 2: return SlotPreviewText_2.Get();
	default: return nullptr;
	}
}

UButton* UYogSlotSelectWidgetBase::GetContinueButton(int32 SlotIndex) const
{
	switch (SlotIndex)
	{
	case 0: return BtnContinue_0.Get();
	case 1: return BtnContinue_1.Get();
	case 2: return BtnContinue_2.Get();
	default: return nullptr;
	}
}

UButton* UYogSlotSelectWidgetBase::GetDeleteButton(int32 SlotIndex) const
{
	switch (SlotIndex)
	{
	case 0: return BtnDelete_0.Get();
	case 1: return BtnDelete_1.Get();
	case 2: return BtnDelete_2.Get();
	default: return nullptr;
	}
}

void UYogSlotSelectWidgetBase::HandleContinueSlot0()
{
	ContinueSlot(0);
}

void UYogSlotSelectWidgetBase::HandleContinueSlot1()
{
	ContinueSlot(1);
}

void UYogSlotSelectWidgetBase::HandleContinueSlot2()
{
	ContinueSlot(2);
}

void UYogSlotSelectWidgetBase::HandleNewGameSlot0()
{
	NewGameSlot(0);
}

void UYogSlotSelectWidgetBase::HandleNewGameSlot1()
{
	NewGameSlot(1);
}

void UYogSlotSelectWidgetBase::HandleNewGameSlot2()
{
	NewGameSlot(2);
}

void UYogSlotSelectWidgetBase::HandleDeleteSlot0()
{
	DeleteSlot(0);
}

void UYogSlotSelectWidgetBase::HandleDeleteSlot1()
{
	DeleteSlot(1);
}

void UYogSlotSelectWidgetBase::HandleDeleteSlot2()
{
	DeleteSlot(2);
}

void UYogSlotSelectWidgetBase::ApplyPreview_0(const FSlotPreviewData& Preview)
{
	ApplyPreview(0, Preview);
}

void UYogSlotSelectWidgetBase::ApplyPreview_1(const FSlotPreviewData& Preview)
{
	ApplyPreview(1, Preview);
}

void UYogSlotSelectWidgetBase::ApplyPreview_2(const FSlotPreviewData& Preview)
{
	ApplyPreview(2, Preview);
}
