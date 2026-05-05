#include "UI/CombatDeckEditCardSlotWidget.h"

#include "Blueprint/DragDropOperation.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/World.h"
#include "InputCoreTypes.h"
#include "TimerManager.h"
#include "UI/CombatDeckEditDragDropOperation.h"
#include "UI/CombatDeckEditWidget.h"
#include "UI/YogCommonRichTextBlock.h"

namespace
{
constexpr float BlockedFeedbackSeconds = 0.22f;
constexpr float DragVisualOpacity = 0.72f;
constexpr float DragVisualScale = 1.04f;
constexpr float SelectedVisualScale = 1.03f;
const FLinearColor SelectedColorAndOpacity(0.84f, 0.88f, 0.96f, 1.0f);
const FVector2D BlockedFeedbackOffset(6.0f, 0.0f);
}

void UCombatDeckEditCardSlotWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetIsFocusable(true);
	CaptureDefaultVisualState();

	if (SelectButton)
	{
		SelectButton->OnClicked.AddDynamic(this, &UCombatDeckEditCardSlotWidget::HandleSelectClicked);
	}
	if (ReverseButton)
	{
		ReverseButton->OnClicked.AddDynamic(this, &UCombatDeckEditCardSlotWidget::HandleReverseClicked);
	}
}

void UCombatDeckEditCardSlotWidget::NativeDestruct()
{
	ResetVisualState();
	Super::NativeDestruct();
}

void UCombatDeckEditCardSlotWidget::SetCard(UCombatDeckEditWidget* InOwnerWidget, const FCombatCardInstance& InCard, int32 InDeckIndex, bool bInSelected)
{
	ResetVisualState();
	OwnerWidget = InOwnerWidget;
	Card = InCard;
	DeckIndex = InDeckIndex;
	bSelected = bInSelected;
	SetVisibility(ESlateVisibility::Visible);

	if (CardIcon)
	{
		UTexture2D* Icon = Card.SourceData ? Card.SourceData->RuneInfo.RuneConfig.RuneIcon : nullptr;
		CardIcon->SetVisibility(Icon ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		if (Icon)
		{
			CardIcon->SetBrushFromTexture(Icon);
		}
	}

	SetTextIfSupported(CardNameText, GetCardDisplayName(Card));
	SetTextIfSupported(TypeText, GetCardTypeText(Card.Config.CardType));
	SetTextIfSupported(DirectionText, GetDirectionText(Card));

	if (DirectionText)
	{
		DirectionText->SetVisibility(Card.Config.CardType == ECombatCardType::Link
			? ESlateVisibility::Visible
			: ESlateVisibility::Collapsed);
	}
	if (ReverseButton)
	{
		ReverseButton->SetVisibility(Card.Config.CardType == ECombatCardType::Link
			? ESlateVisibility::Visible
			: ESlateVisibility::Collapsed);
	}
	if (SelectedMark)
	{
		SelectedMark->SetVisibility(bSelected ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}

	ApplySelectionVisual();
	BP_OnCardChanged(Card, DeckIndex, bSelected);
}

void UCombatDeckEditCardSlotWidget::ClearCard()
{
	OwnerWidget = nullptr;
	Card = FCombatCardInstance();
	DeckIndex = INDEX_NONE;
	bSelected = false;
	SetVisibility(ESlateVisibility::Collapsed);
}

FReply UCombatDeckEditCardSlotWidget::NativeOnPreviewMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		if (IsPointerOverReverseButton(InMouseEvent))
		{
			return Super::NativeOnPreviewMouseButtonDown(InGeometry, InMouseEvent);
		}

		return HandleCardMouseButtonDown(InMouseEvent);
	}

	return Super::NativeOnPreviewMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UCombatDeckEditCardSlotWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		return HandleCardMouseButtonDown(InMouseEvent);
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UCombatDeckEditCardSlotWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		return FReply::Handled().ReleaseMouseCapture();
	}

	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

FReply UCombatDeckEditCardSlotWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	if (Key == EKeys::R || Key == EKeys::Gamepad_FaceButton_Left)
	{
		return TryHandleReverseInput(Key);
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UCombatDeckEditCardSlotWidget::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);
	SelectThisCard();
}

void UCombatDeckEditCardSlotWidget::NativeOnAddedToFocusPath(const FFocusEvent& InFocusEvent)
{
	Super::NativeOnAddedToFocusPath(InFocusEvent);
	UE_LOG(LogTemp, Verbose, TEXT("[CombatDeckInput][CardFocusIgnored] DeckIndex=%d"), DeckIndex);
}

void UCombatDeckEditCardSlotWidget::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation)
{
	if (IsInteractionLocked())
	{
		TriggerBlockedFeedback();
		return;
	}

	UCombatDeckEditDragDropOperation* Operation = NewObject<UCombatDeckEditDragDropOperation>();
	Operation->SourceIndex = DeckIndex;
	Operation->Payload = this;
	OutOperation = Operation;
	if (OwnerWidget)
	{
		OwnerWidget->BeginDragPreview(DeckIndex);
	}
	BP_OnDragStarted(Card, DeckIndex);
}

bool UCombatDeckEditCardSlotWidget::NativeOnDragOver(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	if (!Cast<UCombatDeckEditDragDropOperation>(InOperation))
	{
		return false;
	}

	const int32 InsertIndex = CalculateDropInsertIndex(InGeometry, InDragDropEvent);
	if (OwnerWidget)
	{
		OwnerWidget->UpdateDragPreview(InsertIndex);
	}
	BP_OnDragHovered(InsertIndex);
	return true;
}

bool UCombatDeckEditCardSlotWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	UCombatDeckEditDragDropOperation* Operation = Cast<UCombatDeckEditDragDropOperation>(InOperation);
	if (!Operation || !OwnerWidget || DeckIndex == INDEX_NONE)
	{
		return false;
	}

	if (UCombatDeckEditCardSlotWidget* SourceSlot = Cast<UCombatDeckEditCardSlotWidget>(Operation->Payload))
	{
		SourceSlot->ResetVisualState();
	}

	const bool bMoved = OwnerWidget->CommitDragPreview(CalculateDropInsertIndex(InGeometry, InDragDropEvent));
	ResetVisualState();
	return bMoved;
}

void UCombatDeckEditCardSlotWidget::NativeOnDragCancelled(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	if (OwnerWidget)
	{
		OwnerWidget->EndDragPreview();
	}
	ResetVisualState();
	Super::NativeOnDragCancelled(InDragDropEvent, InOperation);
}

void UCombatDeckEditCardSlotWidget::HandleSelectClicked()
{
	// Selection is driven by hover/focus. Clicks are reserved for drag pickup.
}

void UCombatDeckEditCardSlotWidget::HandleReverseClicked()
{
	SelectThisCard();
	if (IsInteractionLocked())
	{
		TriggerBlockedFeedback();
		return;
	}

	if (OwnerWidget)
	{
		OwnerWidget->ToggleLinkOrientation(DeckIndex);
	}
}

void UCombatDeckEditCardSlotWidget::SelectThisCard()
{
	if (OwnerWidget && OwnerWidget->IsDragPreviewActive())
	{
		return;
	}

	if (OwnerWidget && DeckIndex != INDEX_NONE && OwnerWidget->GetSelectedCardIndex() != DeckIndex)
	{
		OwnerWidget->SelectCard(DeckIndex);
	}
}

bool UCombatDeckEditCardSlotWidget::IsInteractionLocked() const
{
	return OwnerWidget && OwnerWidget->IsInteractionLocked();
}

void UCombatDeckEditCardSlotWidget::TriggerBlockedFeedback()
{
	CaptureDefaultVisualState();
	SetColorAndOpacity(FLinearColor(1.0f, 0.16f, 0.16f, 1.0f));

	FWidgetTransform BlockedTransform = DefaultRenderTransform;
	BlockedTransform.Translation += BlockedFeedbackOffset;
	SetRenderTransform(BlockedTransform);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(BlockedFeedbackTimerHandle);
		World->GetTimerManager().SetTimer(
			BlockedFeedbackTimerHandle,
			this,
			&UCombatDeckEditCardSlotWidget::ResetVisualState,
			BlockedFeedbackSeconds,
			false);
	}

	BP_OnInteractionBlocked(Card, DeckIndex);
	if (OwnerWidget)
	{
		OwnerWidget->BP_OnInteractionBlocked(Card, DeckIndex);
	}
}

void UCombatDeckEditCardSlotWidget::StartDragVisual()
{
	CaptureDefaultVisualState();
	SetRenderOpacity(DragVisualOpacity);

	FWidgetTransform DragTransform = DefaultRenderTransform;
	DragTransform.Scale *= FVector2D(DragVisualScale, DragVisualScale);
	SetRenderTransform(DragTransform);
}

void UCombatDeckEditCardSlotWidget::ResetVisualState()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(BlockedFeedbackTimerHandle);
	}

	if (!bCapturedDefaultVisualState)
	{
		return;
	}

	SetColorAndOpacity(DefaultColorAndOpacity);
	SetRenderOpacity(DefaultRenderOpacity);
	SetRenderTransform(DefaultRenderTransform);
}

void UCombatDeckEditCardSlotWidget::CaptureDefaultVisualState()
{
	if (bCapturedDefaultVisualState)
	{
		return;
	}

	DefaultColorAndOpacity = GetColorAndOpacity();
	DefaultRenderOpacity = GetRenderOpacity();
	DefaultRenderTransform = GetRenderTransform();
	bCapturedDefaultVisualState = true;
}

void UCombatDeckEditCardSlotWidget::ApplySelectionVisual()
{
	if (!bSelected)
	{
		return;
	}

	SetColorAndOpacity(SelectedColorAndOpacity);

	FWidgetTransform SelectedTransform = DefaultRenderTransform;
	SelectedTransform.Scale *= FVector2D(SelectedVisualScale, SelectedVisualScale);
	SetRenderTransform(SelectedTransform);
}

FReply UCombatDeckEditCardSlotWidget::HandleCardMouseButtonDown(const FPointerEvent& InMouseEvent)
{
	MouseDownTimeSeconds = FPlatformTime::Seconds();
	if (IsInteractionLocked())
	{
		TriggerBlockedFeedback();
		return FReply::Handled();
	}

	if (OwnerWidget && DeckIndex != INDEX_NONE)
	{
		OwnerWidget->BeginDragPreview(DeckIndex);
		BP_OnDragStarted(Card, DeckIndex);
	}

	return FReply::Handled();
}

FReply UCombatDeckEditCardSlotWidget::TryHandleReverseInput(const FKey& Key)
{
	if (Key != EKeys::R && Key != EKeys::Gamepad_FaceButton_Left)
	{
		return FReply::Unhandled();
	}

	SelectThisCard();
	if (IsInteractionLocked())
	{
		TriggerBlockedFeedback();
		return FReply::Handled();
	}

	if (OwnerWidget && OwnerWidget->ToggleLinkOrientation(DeckIndex))
	{
		return FReply::Handled();
	}

	return FReply::Unhandled();
}

bool UCombatDeckEditCardSlotWidget::IsPointerOverReverseButton(const FPointerEvent& InMouseEvent) const
{
	if (!ReverseButton || ReverseButton->GetVisibility() == ESlateVisibility::Collapsed)
	{
		return false;
	}

	return ReverseButton->GetCachedGeometry().IsUnderLocation(InMouseEvent.GetScreenSpacePosition());
}

int32 UCombatDeckEditCardSlotWidget::CalculateDropInsertIndex(const FGeometry& InGeometry, const FPointerEvent& ScreenEvent) const
{
	const FVector2D LocalPosition = InGeometry.AbsoluteToLocal(ScreenEvent.GetScreenSpacePosition());
	const bool bInsertAfter = LocalPosition.Y >= InGeometry.GetLocalSize().Y * 0.5f;
	return DeckIndex + (bInsertAfter ? 1 : 0);
}

FText UCombatDeckEditCardSlotWidget::GetCardDisplayName(const FCombatCardInstance& InCard)
{
	if (!InCard.Config.DisplayName.IsEmpty())
	{
		return InCard.Config.DisplayName;
	}
	return InCard.SourceData ? FText::FromName(InCard.SourceData->RuneInfo.RuneConfig.RuneName) : FText::FromString(TEXT("Card"));
}

FText UCombatDeckEditCardSlotWidget::GetCardTypeText(ECombatCardType CardType)
{
	switch (CardType)
	{
	case ECombatCardType::Link:
		return FText::FromString(TEXT("连携"));
	case ECombatCardType::Finisher:
		return FText::FromString(TEXT("终结技"));
	case ECombatCardType::Passive:
		return FText::FromString(TEXT("被动"));
	case ECombatCardType::Normal:
	case ECombatCardType::Attack:
	default:
		return FText::FromString(TEXT("普通"));
	}
}

FText UCombatDeckEditCardSlotWidget::GetDirectionText(const FCombatCardInstance& InCard)
{
	if (InCard.Config.CardType != ECombatCardType::Link)
	{
		return FText::GetEmpty();
	}

	return InCard.LinkOrientation == ECombatCardLinkOrientation::Reversed
		? FText::FromString(TEXT("<-"))
		: FText::FromString(TEXT("->"));
}

void UCombatDeckEditCardSlotWidget::SetTextIfSupported(UWidget* Widget, const FText& Text)
{
	if (UTextBlock* TextBlock = Cast<UTextBlock>(Widget))
	{
		TextBlock->SetText(Text);
		return;
	}

	if (UYogCommonRichTextBlock* RichTextBlock = Cast<UYogCommonRichTextBlock>(Widget))
	{
		RichTextBlock->SetText(Text);
	}
}
