#include "UI/CombatDeckEditWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Character/PlayerCharacterBase.h"
#include "Components/Border.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/PanelWidget.h"
#include "Components/SizeBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/VerticalBox.h"
#include "CommonInputSubsystem.h"
#include "InputCoreTypes.h"
#include "Input/CommonUIInputTypes.h"
#include "Framework/Application/SlateApplication.h"
#include "UI/CombatDeckEditCardSlotWidget.h"
#include "UI/CombatDeckEditDragDropOperation.h"
#include "UI/RuneInfoCardWidget.h"
#include "UI/BackpackStyleDataAsset.h"

namespace
{
FString CombatDeckCardTypeToString(ECombatCardType CardType)
{
	switch (CardType)
	{
	case ECombatCardType::Link:
		return TEXT("Link");
	case ECombatCardType::Finisher:
		return TEXT("Finisher");
	case ECombatCardType::Normal:
		return TEXT("Normal");
	case ECombatCardType::Attack:
		return TEXT("LegacyAttack");
	default:
		return TEXT("Unknown");
	}
}

FString DescribeCombatDeckCard(const FCombatCardInstance& Card)
{
	FString DisplayName = Card.Config.DisplayName.IsEmpty() ? FString() : Card.Config.DisplayName.ToString();
	if (DisplayName.IsEmpty() && Card.SourceData)
	{
		DisplayName = Card.SourceData->GetRuneName().ToString();
	}
	if (DisplayName.IsEmpty())
	{
		DisplayName = TEXT("<EmptyName>");
	}

	return FString::Printf(TEXT("Name=%s Type=%s Id=%s Guid=%s"),
		*DisplayName,
		*CombatDeckCardTypeToString(Card.Config.CardType),
		*Card.Config.CardIdTag.ToString(),
		*Card.InstanceGuid.ToString(EGuidFormats::Digits));
}
}

void UCombatDeckEditWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetIsFocusable(true);
	BindToOwningPlayerCombatDeck();
}

void UCombatDeckEditWidget::NativeDestruct()
{
	HideGamepadFloatingDragCard();
	UnbindFromCurrentDeck();
	Super::NativeDestruct();
}

void UCombatDeckEditWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!bDragPreviewActive || bGamepadDragActive || !BoundCombatDeck || !CardListBox || !FSlateApplication::IsInitialized())
	{
		return;
	}

	const FVector2D CursorScreenPosition = FSlateApplication::Get().GetCursorPos();
	UpdateDragPreview(GetDropInsertIndexFromListGeometry(MyGeometry, CursorScreenPosition));

	if (!bDragPreviewActive)
	{
		return;
	}

	const bool bSlateLeftMouseDown = FSlateApplication::Get().GetPressedMouseButtons().Contains(EKeys::LeftMouseButton);
	const bool bControllerLeftMouseDown = GetOwningPlayer() && GetOwningPlayer()->IsInputKeyDown(EKeys::LeftMouseButton);
	if (!bSlateLeftMouseDown && !bControllerLeftMouseDown)
	{
		const int32 CommitInsertIndex = DragPreviewInsertIndex;
		UE_LOG(LogTemp, Warning, TEXT("[CombatDeckInput][MouseReleaseFallback] Commit Insert=%d Source=%d Cursor=(%.1f,%.1f) SlateDown=%d ControllerDown=%d"),
			CommitInsertIndex,
			DragSourceIndex,
			CursorScreenPosition.X,
			CursorScreenPosition.Y,
			bSlateLeftMouseDown ? 1 : 0,
			bControllerLeftMouseDown ? 1 : 0);
		CommitDragPreview(CommitInsertIndex);
	}
}

void UCombatDeckEditWidget::BindToOwningPlayerCombatDeck()
{
	APlayerCharacterBase* Player = Cast<APlayerCharacterBase>(GetOwningPlayerPawn());
	BindToCombatDeck(Player ? Player->CombatDeckComponent : nullptr);
}

void UCombatDeckEditWidget::BindToCombatDeck(UCombatDeckComponent* InCombatDeck)
{
	UE_LOG(LogTemp, Warning, TEXT("[CombatDeckInput][Bind] OldDeck=%s NewDeck=%s"),
		*GetNameSafe(BoundCombatDeck),
		*GetNameSafe(InCombatDeck));

	if (BoundCombatDeck == InCombatDeck)
	{
		RefreshDeckList();
		return;
	}

	UnbindFromCurrentDeck();
	BoundCombatDeck = InCombatDeck;

	if (BoundCombatDeck)
	{
		BoundCombatDeck->OnDeckLoaded.AddDynamic(this, &UCombatDeckEditWidget::HandleDeckChanged);
		BoundCombatDeck->OnShuffleCompleted.AddDynamic(this, &UCombatDeckEditWidget::HandleDeckChanged);
		BoundCombatDeck->OnCardConsumed.AddDynamic(this, &UCombatDeckEditWidget::HandleCardConsumed);
		BoundCombatDeck->OnRewardAddedToDeck.AddDynamic(this, &UCombatDeckEditWidget::HandleRewardAddedToDeck);
	}

	SelectedCardIndex = INDEX_NONE;
	RefreshDeckList();
}

void UCombatDeckEditWidget::UnbindFromCurrentDeck()
{
	if (!BoundCombatDeck)
	{
		return;
	}

	BoundCombatDeck->OnDeckLoaded.RemoveDynamic(this, &UCombatDeckEditWidget::HandleDeckChanged);
	BoundCombatDeck->OnShuffleCompleted.RemoveDynamic(this, &UCombatDeckEditWidget::HandleDeckChanged);
	BoundCombatDeck->OnCardConsumed.RemoveDynamic(this, &UCombatDeckEditWidget::HandleCardConsumed);
	BoundCombatDeck->OnRewardAddedToDeck.RemoveDynamic(this, &UCombatDeckEditWidget::HandleRewardAddedToDeck);
	BoundCombatDeck = nullptr;
}

void UCombatDeckEditWidget::RefreshDeckList()
{
	RefreshDeckListInternal(bDragPreviewActive);
}

TArray<ECombatDeckEditCardLinkHintState> UCombatDeckEditWidget::BuildLinkHintStatesForDeck(const TArray<FCombatCardInstance>& Cards)
{
	TArray<ECombatDeckEditCardLinkHintState> HintStates;
	HintStates.Init(ECombatDeckEditCardLinkHintState::None, Cards.Num());

	auto AssignHintState = [&HintStates](const int32 CardIndex, const ECombatDeckEditCardLinkHintState NewState)
	{
		if (!HintStates.IsValidIndex(CardIndex))
		{
			return;
		}

		const bool bReversedHint =
			NewState == ECombatDeckEditCardLinkHintState::ReversedLink ||
			NewState == ECombatDeckEditCardLinkHintState::ReversedTarget;
		if (HintStates[CardIndex] == ECombatDeckEditCardLinkHintState::None || bReversedHint)
		{
			HintStates[CardIndex] = NewState;
		}
	};

	for (int32 Index = 0; Index < Cards.Num(); ++Index)
	{
		const FCombatCardInstance& Card = Cards[Index];
		if (Card.Config.CardType != ECombatCardType::Link)
		{
			continue;
		}

		const bool bReversed = Card.LinkOrientation == ECombatCardLinkOrientation::Reversed;
		const int32 TargetIndex = bReversed ? Index + 1 : Index - 1;
		if (!Cards.IsValidIndex(TargetIndex))
		{
			continue;
		}

		AssignHintState(Index, bReversed
			? ECombatDeckEditCardLinkHintState::ReversedLink
			: ECombatDeckEditCardLinkHintState::ForwardLink);
		AssignHintState(TargetIndex, bReversed
			? ECombatDeckEditCardLinkHintState::ReversedTarget
			: ECombatDeckEditCardLinkHintState::ForwardTarget);
	}

	return HintStates;
}

void UCombatDeckEditWidget::SetExternalDetailInfoCard(URuneInfoCardWidget* InInfoCard)
{
	ExternalDetailInfoCard = InInfoCard;
	RefreshSelectedCardInfo();
}

bool UCombatDeckEditWidget::IsUsingExternalDetailInfoCard(const URuneInfoCardWidget* InInfoCard) const
{
	return InInfoCard && ExternalDetailInfoCard.Get() == InInfoCard;
}

void UCombatDeckEditWidget::RefreshDeckListInternal(bool bUseDragPreview)
{
	if (!CardListBox)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CombatDeckInput][Refresh] Failed: CardListBox=null"));
		RefreshSelectedCardInfo();
		return;
	}

	CardListBox->ClearChildren();
	if (!BoundCombatDeck || !CardSlotClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CombatDeckInput][Refresh] Failed: BoundDeck=%s CardSlotClass=%s"),
			*GetNameSafe(BoundCombatDeck),
			*GetNameSafe(CardSlotClass));
		RefreshSelectedCardInfo();
		return;
	}

	const TArray<FCombatCardInstance> Cards = BoundCombatDeck->GetFullDeckSnapshot();
	UE_LOG(LogTemp, Warning, TEXT("[CombatDeckInput][Refresh] Count=%d Selected=%d Preview=%d DragSource=%d Insert=%d Visible=%d"),
		Cards.Num(),
		SelectedCardIndex,
		bUseDragPreview ? 1 : 0,
		DragSourceIndex,
		DragPreviewInsertIndex,
		IsVisible() ? 1 : 0);

	const bool bCanPreviewDrag = bUseDragPreview && Cards.IsValidIndex(DragSourceIndex);
	const int32 VisualInsertIndex = bCanPreviewDrag ? GetPreviewVisualInsertIndex(DragPreviewInsertIndex) : INDEX_NONE;
	const TArray<ECombatDeckEditCardLinkHintState> LinkHintStates = BuildLinkHintStatesForDeck(Cards);
	if (!Cards.IsValidIndex(SelectedCardIndex))
	{
		SelectedCardIndex = Cards.IsEmpty() ? INDEX_NONE : 0;
	}

	int32 VisibleCardIndex = 0;
	bool bAddedDragVisual = false;
	for (int32 Index = 0; Index < Cards.Num(); ++Index)
	{
		if (bCanPreviewDrag && !bAddedDragVisual && VisualInsertIndex == VisibleCardIndex)
		{
			AddDropIndicatorToList();
			AddInlineFloatingDragCardToList(Cards[DragSourceIndex], DragSourceIndex);
			bAddedDragVisual = true;
		}

		if (bCanPreviewDrag && Index == DragSourceIndex)
		{
			continue;
		}

		UCombatDeckEditCardSlotWidget* CardSlotWidget = CreateWidget<UCombatDeckEditCardSlotWidget>(GetOwningPlayer(), CardSlotClass);
		if (!CardSlotWidget)
		{
			continue;
		}

		const bool bShowSelectedState = !bCanPreviewDrag && Index == SelectedCardIndex;
		if (bShowSelectedState && PendingSelectionAnimationDirection != 0)
		{
			CardSlotWidget->SetSelectionAnimationHint(PendingSelectionAnimationDirection);
		}
		CardSlotWidget->SetCard(this, Cards[Index], Index, bShowSelectedState);
		CardSlotWidget->SetLinkHintState(LinkHintStates.IsValidIndex(Index)
			? LinkHintStates[Index]
			: ECombatDeckEditCardLinkHintState::None);
		AddWidgetToCardList(WrapDeckCardWidget(CardSlotWidget),
			IsCardListHorizontal() ? FMargin(0.0f, 0.0f, GetDeckCardSpacing(), 0.0f) : FMargin());
		++VisibleCardIndex;
	}

	if (bCanPreviewDrag && !bAddedDragVisual && VisualInsertIndex == VisibleCardIndex)
	{
		AddDropIndicatorToList();
		AddInlineFloatingDragCardToList(Cards[DragSourceIndex], DragSourceIndex);
	}

	PendingSelectionAnimationDirection = 0;
	RefreshSelectedCardInfo();
}

FReply UCombatDeckEditWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	if (Key == EKeys::R || Key == EKeys::Gamepad_FaceButton_Left)
	{
		if (ToggleSelectedLinkOrientation())
		{
			return FReply::Handled();
		}
	}
	if (Key == EKeys::F || Key == EKeys::Gamepad_FaceButton_Top)
	{
		ToggleDetailPreview();
		return FReply::Handled();
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UCombatDeckEditWidget::SelectCard(int32 CardIndex)
{
	const TArray<FCombatCardInstance> Cards = BoundCombatDeck ? BoundCombatDeck->GetFullDeckSnapshot() : TArray<FCombatCardInstance>();
	const int32 OldSelectedIndex = SelectedCardIndex;
	if (!Cards.IsValidIndex(CardIndex))
	{
		SelectedCardIndex = INDEX_NONE;
	}
	else
	{
		SelectedCardIndex = CardIndex;
	}

	const FString CardDesc = Cards.IsValidIndex(SelectedCardIndex)
		? DescribeCombatDeckCard(Cards[SelectedCardIndex])
		: FString(TEXT("<Invalid>"));
	UE_LOG(LogTemp, Warning, TEXT("[CombatDeckInput][SelectCard] Request=%d Old=%d New=%d Count=%d Card={%s}"),
		CardIndex,
		OldSelectedIndex,
		SelectedCardIndex,
		Cards.Num(),
		*CardDesc);

	RefreshDeckList();
}

bool UCombatDeckEditWidget::SelectAdjacentCard(int32 Direction)
{
	if (Direction == 0)
	{
		return CanHandleDeckInput();
	}

	const TArray<FCombatCardInstance> Cards = BoundCombatDeck ? BoundCombatDeck->GetFullDeckSnapshot() : TArray<FCombatCardInstance>();
	if (Cards.IsEmpty())
	{
		return false;
	}

	const int32 CurrentIndex = Cards.IsValidIndex(SelectedCardIndex) ? SelectedCardIndex : 0;
	const int32 NextIndex = FMath::Clamp(CurrentIndex + Direction, 0, Cards.Num() - 1);
	UE_LOG(LogTemp, Warning, TEXT("[CombatDeckInput][SelectAdjacent] Dir=%d Current=%d Next=%d Count=%d CurrentCard={%s} NextCard={%s}"),
		Direction,
		CurrentIndex,
		NextIndex,
		Cards.Num(),
		Cards.IsValidIndex(CurrentIndex) ? *DescribeCombatDeckCard(Cards[CurrentIndex]) : TEXT("<Invalid>"),
		Cards.IsValidIndex(NextIndex) ? *DescribeCombatDeckCard(Cards[NextIndex]) : TEXT("<Invalid>"));
	if (NextIndex == SelectedCardIndex)
	{
		return true;
	}

	PendingSelectionAnimationDirection = Direction;
	SelectCard(NextIndex);
	return true;
}

bool UCombatDeckEditWidget::HandleDeckDirectionalInput(int32 Direction)
{
	NotifyGamepadNavigationInput();

	if (!CanHandleDeckInput())
	{
		UE_LOG(LogTemp, Warning, TEXT("[CombatDeckInput][Directional] Rejected CanHandle=0 Direction=%d BoundDeck=%s Visible=%d"),
			Direction,
			*GetNameSafe(BoundCombatDeck),
			IsVisible() ? 1 : 0);
		return false;
	}

	if (bGamepadDragActive)
	{
		const TArray<FCombatCardInstance> Cards = BoundCombatDeck ? BoundCombatDeck->GetFullDeckSnapshot() : TArray<FCombatCardInstance>();
		const int32 OldVisual = GetPreviewVisualInsertIndex(GamepadDragInsertIndex);
		int32 NewInsert = FMath::Clamp(GamepadDragInsertIndex + Direction, 0, Cards.Num());
		// Insert positions Source and Source+1 both map to the same visual slot, so the very
		// first directional step out of rest would otherwise produce no visible movement.
		// Skip past that dead zone by taking one more step in the same direction when the
		// visual index hasn't actually changed.
		if (NewInsert != GamepadDragInsertIndex && GetPreviewVisualInsertIndex(NewInsert) == OldVisual)
		{
			const int32 ExtraInsert = FMath::Clamp(NewInsert + Direction, 0, Cards.Num());
			if (ExtraInsert != NewInsert)
			{
				NewInsert = ExtraInsert;
			}
		}
		GamepadDragInsertIndex = NewInsert;
		UE_LOG(LogTemp, Warning, TEXT("[CombatDeckInput][DirectionalDrag] Dir=%d NewInsert=%d Count=%d Source=%d"),
			Direction,
			GamepadDragInsertIndex,
			Cards.Num(),
			DragSourceIndex);
		UpdateDragPreview(GamepadDragInsertIndex);
		return true;
	}

	UE_LOG(LogTemp, Warning, TEXT("[CombatDeckInput][DirectionalSelect] Dir=%d Selected=%d"), Direction, SelectedCardIndex);
	return SelectAdjacentCard(Direction);
}

bool UCombatDeckEditWidget::HandleDeckSelectPressed()
{
	NotifyGamepadNavigationInput();

	if (!CanHandleDeckInput() || bInteractionLocked)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CombatDeckInput][A_Pressed] Rejected CanHandle=%d Locked=%d BoundDeck=%s Visible=%d"),
			CanHandleDeckInput() ? 1 : 0,
			bInteractionLocked ? 1 : 0,
			*GetNameSafe(BoundCombatDeck),
			IsVisible() ? 1 : 0);
		return false;
	}

	if (bGamepadDragActive)
	{
		const int32 CommitInsertIndex = GamepadDragInsertIndex;
		UE_LOG(LogTemp, Warning, TEXT("[CombatDeckInput][A_PressedCommit] Source=%d Insert=%d"),
			DragSourceIndex,
			CommitInsertIndex);
		ResetGamepadDragState();
		return CommitDragPreview(CommitInsertIndex);
	}

	if (!EnsureValidSelection())
	{
		return false;
	}

	bGamepadSelectHeld = true;
	GamepadSelectHeldTime = 0.0f;
	GamepadDragInsertIndex = SelectedCardIndex;
	UE_LOG(LogTemp, Warning, TEXT("[CombatDeckInput][A_PressedPickUp] Selected=%d Insert=%d"), SelectedCardIndex, GamepadDragInsertIndex);
	BeginGamepadDrag();
	return bGamepadDragActive;
}

bool UCombatDeckEditWidget::HandleDeckSelectReleased()
{
	if (bGamepadDragActive)
	{
		const int32 CommitInsertIndex = GamepadDragInsertIndex;
		bGamepadSelectHeld = false;
		UE_LOG(LogTemp, Warning, TEXT("[CombatDeckInput][A_ReleasedDrag] Source=%d Insert=%d"),
			DragSourceIndex,
			CommitInsertIndex);

		ResetGamepadDragState();
		return CommitDragPreview(CommitInsertIndex);
	}

	if (!bGamepadSelectHeld && !bGamepadDragActive)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CombatDeckInput][A_Released] Ignored Held=0 Drag=0 Selected=%d"), SelectedCardIndex);
		return true;
	}

	UE_LOG(LogTemp, Warning, TEXT("[CombatDeckInput][A_ReleasedSelect] Selected=%d HeldTime=%.3f"), SelectedCardIndex, GamepadSelectHeldTime);
	ResetGamepadDragState();
	EnsureValidSelection();
	return true;
}

bool UCombatDeckEditWidget::CancelDeckGamepadDrag()
{
	if (!bGamepadDragActive && !bDragPreviewActive)
	{
		return false;
	}

	UE_LOG(LogTemp, Warning, TEXT("[CombatDeckInput][CancelGamepadDrag] Source=%d Insert=%d"),
		DragSourceIndex,
		GamepadDragInsertIndex);
	ResetGamepadDragState();
	EndDragPreview();
	EnsureValidSelection();
	return true;
}

void UCombatDeckEditWidget::TickDeckGamepadInput(float DeltaTime)
{
	if (bGamepadSelectHeld && bGamepadDragActive)
	{
		GamepadSelectHeldTime += DeltaTime;
	}
}

bool UCombatDeckEditWidget::ToggleDetailPreview()
{
	SetDetailPreviewVisible(!bDetailPreviewVisible);
	return bDetailPreviewVisible;
}

void UCombatDeckEditWidget::SetDetailPreviewVisible(bool bVisible)
{
	if (bDetailPreviewVisible == bVisible)
	{
		ApplyDetailPreviewVisibility();
		return;
	}

	bDetailPreviewVisible = bVisible;
	UE_LOG(LogTemp, Warning, TEXT("[CombatDeckInput][DetailPreview] Visible=%d"), bDetailPreviewVisible ? 1 : 0);
	RefreshSelectedCardInfo();
}

bool UCombatDeckEditWidget::MoveCard(int32 FromIndex, int32 InsertIndex)
{
	if (!BoundCombatDeck || bInteractionLocked)
	{
		return false;
	}

	const bool bMoved = BoundCombatDeck->MoveCardInDeck(FromIndex, InsertIndex);
	UE_LOG(LogTemp, Warning, TEXT("[CombatDeckInput][MoveCard] From=%d Insert=%d Moved=%d"),
		FromIndex,
		InsertIndex,
		bMoved ? 1 : 0);
	if (bMoved)
	{
		SelectedCardIndex = FromIndex < InsertIndex ? InsertIndex - 1 : InsertIndex;
		RefreshDeckList();
	}
	return bMoved;
}

void UCombatDeckEditWidget::BeginDragPreview(int32 SourceIndex)
{
	if (!BoundCombatDeck || bInteractionLocked)
	{
		return;
	}

	const TArray<FCombatCardInstance> Cards = BoundCombatDeck->GetFullDeckSnapshot();
	if (!Cards.IsValidIndex(SourceIndex))
	{
		return;
	}

	bDragPreviewActive = true;
	DragSourceIndex = SourceIndex;
	DragPreviewInsertIndex = SourceIndex;
	SelectedCardIndex = SourceIndex;
	UE_LOG(LogTemp, Warning, TEXT("[CombatDeckInput][BeginDragPreview] Source=%d Card={%s}"),
		SourceIndex,
		Cards.IsValidIndex(SourceIndex) ? *DescribeCombatDeckCard(Cards[SourceIndex]) : TEXT("<Invalid>"));
	RefreshDeckListInternal(true);
}

void UCombatDeckEditWidget::UpdateDragPreview(int32 InsertIndex)
{
	if (!bDragPreviewActive || !BoundCombatDeck)
	{
		return;
	}

	const TArray<FCombatCardInstance> Cards = BoundCombatDeck->GetFullDeckSnapshot();
	const int32 ClampedInsertIndex = FMath::Clamp(InsertIndex, 0, Cards.Num());
	if (DragPreviewInsertIndex == ClampedInsertIndex)
	{
		return;
	}

	DragPreviewInsertIndex = ClampedInsertIndex;
	UE_LOG(LogTemp, Warning, TEXT("[CombatDeckInput][UpdateDragPreview] Insert=%d Source=%d"), DragPreviewInsertIndex, DragSourceIndex);
	RefreshDeckListInternal(true);
}

bool UCombatDeckEditWidget::CommitDragPreview(int32 InsertIndex)
{
	if (!bDragPreviewActive || !BoundCombatDeck || !BoundCombatDeck->GetFullDeckSnapshot().IsValidIndex(DragSourceIndex))
	{
		UE_LOG(LogTemp, Warning, TEXT("[CombatDeckInput][CommitDragPreview] Ignored Active=%d Deck=%s Source=%d Insert=%d"),
			bDragPreviewActive ? 1 : 0,
			*GetNameSafe(BoundCombatDeck),
			DragSourceIndex,
			InsertIndex);
		return false;
	}

	const int32 SourceIndex = DragSourceIndex;
	const int32 TargetInsertIndex = BoundCombatDeck
		? FMath::Clamp(InsertIndex, 0, BoundCombatDeck->GetFullDeckSnapshot().Num())
		: InsertIndex;
	const bool bNoOpDrop = SourceIndex == TargetInsertIndex || SourceIndex + 1 == TargetInsertIndex;

	UE_LOG(LogTemp, Warning, TEXT("[CombatDeckInput][CommitDragPreview] Source=%d Insert=%d Target=%d"),
		SourceIndex,
		InsertIndex,
		TargetInsertIndex);

	if (bNoOpDrop)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CombatDeckInput][CommitDragPreview] NoOpDrop Source=%d Target=%d"), SourceIndex, TargetInsertIndex);
		EndDragPreview();
		EnsureValidSelection();
		return true;
	}

	EndDragPreview();
	return MoveCard(SourceIndex, TargetInsertIndex);
}

void UCombatDeckEditWidget::EndDragPreview()
{
	if (!bDragPreviewActive)
	{
		return;
	}

	bDragPreviewActive = false;
	DragSourceIndex = INDEX_NONE;
	DragPreviewInsertIndex = INDEX_NONE;
	HideGamepadFloatingDragCard();
	RefreshDeckListInternal(false);

	// ClearChildren 销毁了所有旧卡槽 Widget，Slate 焦点随之丢失。
	// 主动把焦点收回本 Widget，确保它仍在 BackpackScreenWidget 的焦点路径内，
	// 否则 BackpackScreenWidget 的 NativeOnPreviewKeyDown 收不到后续手柄按键。
	SetKeyboardFocus();
}

bool UCombatDeckEditWidget::ToggleLinkOrientation(int32 CardIndex)
{
	if (!BoundCombatDeck || bInteractionLocked)
	{
		return false;
	}

	const bool bChanged = BoundCombatDeck->ToggleCardLinkOrientationByIndex(CardIndex);
	if (bChanged)
	{
		SelectedCardIndex = CardIndex;
		RefreshDeckList();
	}
	return bChanged;
}

bool UCombatDeckEditWidget::ToggleSelectedLinkOrientation()
{
	return ToggleLinkOrientation(SelectedCardIndex);
}

bool UCombatDeckEditWidget::CanHandleDeckInput() const
{
	return IsVisible() && BoundCombatDeck && !BoundCombatDeck->GetFullDeckSnapshot().IsEmpty();
}

void UCombatDeckEditWidget::NotifyGamepadNavigationInput()
{
	bPointerHoverSelectionEnabled = false;
	GamepadNavigationSuppressPointerUntilTime = (GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f) + 0.75f;
	if (ULocalPlayer* LocalPlayer = GetOwningLocalPlayer())
	{
		if (UCommonInputSubsystem* CommonInput = LocalPlayer->GetSubsystem<UCommonInputSubsystem>())
		{
			CommonInput->SetCurrentInputType(ECommonInputType::Gamepad);
		}
	}
}

void UCombatDeckEditWidget::NotifyPointerNavigationInput()
{
	if (IsSuppressingPointerInput())
	{
		return;
	}

	if (ULocalPlayer* LocalPlayer = GetOwningLocalPlayer())
	{
		if (UCommonInputSubsystem* CommonInput = LocalPlayer->GetSubsystem<UCommonInputSubsystem>())
		{
			if (CommonInput->GetCurrentInputType() == ECommonInputType::Gamepad)
			{
				return;
			}
		}
	}

	bPointerHoverSelectionEnabled = true;
}

bool UCombatDeckEditWidget::ShouldSelectCardsOnPointerHover() const
{
	if (IsSuppressingPointerInput())
	{
		return false;
	}

	if (ULocalPlayer* LocalPlayer = GetOwningLocalPlayer())
	{
		if (UCommonInputSubsystem* CommonInput = LocalPlayer->GetSubsystem<UCommonInputSubsystem>())
		{
			if (CommonInput->GetCurrentInputType() == ECommonInputType::Gamepad)
			{
				return false;
			}
		}
	}

	return bPointerHoverSelectionEnabled && !bGamepadDragActive;
}

bool UCombatDeckEditWidget::IsSuppressingPointerInput() const
{
	const float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	return CurrentTime < GamepadNavigationSuppressPointerUntilTime;
}

bool UCombatDeckEditWidget::EnsureValidSelection()
{
	const TArray<FCombatCardInstance> Cards = BoundCombatDeck ? BoundCombatDeck->GetFullDeckSnapshot() : TArray<FCombatCardInstance>();
	if (Cards.IsEmpty())
	{
		SelectedCardIndex = INDEX_NONE;
		return false;
	}

	if (!Cards.IsValidIndex(SelectedCardIndex))
	{
		SelectedCardIndex = 0;
		RefreshDeckList();
	}
	return true;
}

void UCombatDeckEditWidget::BeginGamepadDrag()
{
	if (!EnsureValidSelection() || bInteractionLocked)
	{
		ResetGamepadDragState();
		return;
	}

	bGamepadDragActive = true;
	GamepadDragInsertIndex = SelectedCardIndex;
	UE_LOG(LogTemp, Warning, TEXT("[CombatDeckInput][BeginGamepadDrag] Selected=%d Insert=%d"), SelectedCardIndex, GamepadDragInsertIndex);
	BeginDragPreview(SelectedCardIndex);
	if (!bDragPreviewActive)
	{
		ResetGamepadDragState();
	}
}

void UCombatDeckEditWidget::ResetGamepadDragState()
{
	bGamepadSelectHeld = false;
	bGamepadDragActive = false;
	GamepadSelectHeldTime = 0.0f;
	GamepadDragInsertIndex = INDEX_NONE;
}

void UCombatDeckEditWidget::ShowGamepadFloatingDragCard(const FCombatCardInstance& Card, int32 CardIndex)
{
	UE_LOG(LogTemp, Warning, TEXT("[CombatDeckInput][FloatingDrag] ShowRequest Index=%d CardSlotClass=%s OwnerPlayer=%s Card={%s}"),
		CardIndex,
		*GetNameSafe(CardSlotClass),
		*GetNameSafe(GetOwningPlayer()),
		*DescribeCombatDeckCard(Card));

	if (!CardSlotClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CombatDeckInput][FloatingDrag] ShowFailed Reason=CardSlotClassNull"));
		return;
	}

	HideGamepadFloatingDragCard();

	GamepadFloatingDragSlot = CreateWidget<UCombatDeckEditCardSlotWidget>(GetOwningPlayer(), CardSlotClass);
	if (!GamepadFloatingDragSlot)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CombatDeckInput][FloatingDrag] ShowFailed Reason=CreateWidgetFailed Class=%s"),
			*GetNameSafe(CardSlotClass));
		return;
	}

	GamepadFloatingDragSlot->SetCard(this, Card, CardIndex, true);
	GamepadFloatingDragSlot->SetVisibility(ESlateVisibility::HitTestInvisible);
	GamepadFloatingDragSlot->SetRenderOpacity(GamepadFloatingDragOpacity);

	FWidgetTransform FloatingTransform;
	FloatingTransform.Scale = FVector2D(GamepadFloatingDragScale, GamepadFloatingDragScale);
	GamepadFloatingDragSlot->SetRenderTransform(FloatingTransform);
	GamepadFloatingDragSlot->SetAlignmentInViewport(FVector2D(0.0f, 0.5f));
	GamepadFloatingDragSlot->AddToPlayerScreen(10000);
	GamepadFloatingDragSlot->ForceLayoutPrepass();
	UpdateGamepadFloatingDragCardPosition();

	UE_LOG(LogTemp, Warning, TEXT("[CombatDeckInput][FloatingDrag] ShowSuccess Index=%d Slot=%s Visibility=%d Opacity=%.2f Scale=%.2f Desired=(%.1f,%.1f)"),
		CardIndex,
		*GetNameSafe(GamepadFloatingDragSlot),
		static_cast<int32>(GamepadFloatingDragSlot->GetVisibility()),
		GamepadFloatingDragOpacity,
		GamepadFloatingDragScale,
		GamepadFloatingDragSlot->GetDesiredSize().X,
		GamepadFloatingDragSlot->GetDesiredSize().Y);
}

void UCombatDeckEditWidget::UpdateGamepadFloatingDragCardPosition()
{
	if (!GamepadFloatingDragSlot || !CardListBox || !BoundCombatDeck)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CombatDeckInput][FloatingDrag] PositionSkipped Slot=%s List=%s Deck=%s"),
			*GetNameSafe(GamepadFloatingDragSlot),
			*GetNameSafe(CardListBox),
			*GetNameSafe(BoundCombatDeck));
		return;
	}

	const TArray<FCombatCardInstance> Cards = BoundCombatDeck->GetFullDeckSnapshot();
	const FGeometry ListGeometry = CardListBox->GetCachedGeometry();
	const FVector2D ListAbsPosition = ListGeometry.GetAbsolutePosition();
	const FVector2D ListAbsSize = ListGeometry.GetAbsoluteSize();
	const int32 VisualInsertIndex = bDragPreviewActive ? GetPreviewVisualInsertIndex(DragPreviewInsertIndex) : SelectedCardIndex;
	const int32 SlotCount = FMath::Max(1, Cards.Num());
	const bool bHorizontal = IsCardListHorizontal();
	const float SlotExtent = bHorizontal
		? (ListAbsSize.X > 1.0f ? ListAbsSize.X / static_cast<float>(SlotCount) : 190.0f)
		: (ListAbsSize.Y > 1.0f ? ListAbsSize.Y / static_cast<float>(SlotCount) : 52.0f);
	const float ClampedPrimary = FMath::Clamp(static_cast<float>(VisualInsertIndex), 0.0f, static_cast<float>(SlotCount - 1)) * SlotExtent + SlotExtent * 0.5f;
	const FVector2D Position = bHorizontal
		? FVector2D(ListAbsPosition.X + ClampedPrimary + GamepadFloatingDragOffset.X, ListAbsPosition.Y + ListAbsSize.Y * 0.5f + GamepadFloatingDragOffset.Y)
		: FVector2D(ListAbsPosition.X + GamepadFloatingDragOffset.X, ListAbsPosition.Y + ClampedPrimary + GamepadFloatingDragOffset.Y);
	const FVector2D DesiredSize = bHorizontal
		? FVector2D(GetDeckCardWidth(), GetDeckCardHeight())
		: FVector2D(FMath::Max(220.0f, ListAbsSize.X), FMath::Max(44.0f, SlotExtent * 0.92f));
	const float ViewportScale = FMath::Max(0.01f, UWidgetLayoutLibrary::GetViewportScale(this));
	const FVector2D ViewportPosition = Position / ViewportScale;
	const FVector2D ViewportDesiredSize = DesiredSize / ViewportScale;

	GamepadFloatingDragSlot->SetDesiredSizeInViewport(ViewportDesiredSize);
	GamepadFloatingDragSlot->SetPositionInViewport(ViewportPosition, false);
	GamepadFloatingDragSlot->ForceLayoutPrepass();

	UE_LOG(LogTemp, Warning, TEXT("[CombatDeckInput][FloatingDrag] Position Index=%d VisualInsert=%d Source=%d Horizontal=%d ViewportScale=%.3f RemoveDPIScale=0 ListAbs=(%.1f,%.1f) ListSize=(%.1f,%.1f) SlotExtent=%.1f SlatePos=(%.1f,%.1f) ViewportPos=(%.1f,%.1f) Desired=(%.1f,%.1f) ViewportDesired=(%.1f,%.1f) Offset=(%.1f,%.1f)"),
		SelectedCardIndex,
		VisualInsertIndex,
		DragSourceIndex,
		bHorizontal ? 1 : 0,
		ViewportScale,
		ListAbsPosition.X,
		ListAbsPosition.Y,
		ListAbsSize.X,
		ListAbsSize.Y,
		SlotExtent,
		Position.X,
		Position.Y,
		ViewportPosition.X,
		ViewportPosition.Y,
		DesiredSize.X,
		DesiredSize.Y,
		ViewportDesiredSize.X,
		ViewportDesiredSize.Y,
		GamepadFloatingDragOffset.X,
		GamepadFloatingDragOffset.Y);
}

void UCombatDeckEditWidget::HideGamepadFloatingDragCard()
{
	if (!GamepadFloatingDragSlot)
	{
		return;
	}

	GamepadFloatingDragSlot->RemoveFromParent();
	GamepadFloatingDragSlot = nullptr;
	UE_LOG(LogTemp, Warning, TEXT("[CombatDeckInput][FloatingDrag] Hide"));
}

void UCombatDeckEditWidget::SetInteractionLocked(bool bLocked)
{
	bInteractionLocked = bLocked;
}

void UCombatDeckEditWidget::RefreshSelectedCardInfo()
{
	const TArray<FCombatCardInstance> Cards = BoundCombatDeck ? BoundCombatDeck->GetFullDeckSnapshot() : TArray<FCombatCardInstance>();
	const FCombatCardInstance SelectedCard = Cards.IsValidIndex(SelectedCardIndex) ? Cards[SelectedCardIndex] : FCombatCardInstance();

	if (URuneInfoCardWidget* DetailInfoCard = GetActiveDetailInfoCard())
	{
		ApplyDetailPreviewVisibility();
		if (!bDetailPreviewVisible)
		{
			DetailInfoCard->HideCard();
		}
		else if (SelectedCard.SourceData)
		{
			DetailInfoCard->SetVisibility(ESlateVisibility::Visible);
			DetailInfoCard->ShowCombatCard(SelectedCard);
		}
		else
		{
			DetailInfoCard->HideCard();
		}
	}

	BP_OnSelectedCardChanged(SelectedCard, SelectedCardIndex);
}

void UCombatDeckEditWidget::ApplyDetailPreviewVisibility()
{
	URuneInfoCardWidget* DetailInfoCard = GetActiveDetailInfoCard();
	if (!DetailInfoCard)
	{
		return;
	}

	DetailInfoCard->SetVisibility(bDetailPreviewVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
}

URuneInfoCardWidget* UCombatDeckEditWidget::GetActiveDetailInfoCard() const
{
	return ExternalDetailInfoCard ? ExternalDetailInfoCard.Get() : RuneInfoCard.Get();
}

void UCombatDeckEditWidget::HandleDeckChanged(const TArray<FCombatCardInstance>& ActiveSequence)
{
	RefreshDeckList();
}

void UCombatDeckEditWidget::HandleCardConsumed(const FCombatCardInstance& Card, const FCombatCardResolveResult& Result)
{
	RefreshDeckList();
}

void UCombatDeckEditWidget::HandleRewardAddedToDeck(const FCombatCardInstance& Card)
{
	RefreshDeckList();
}

bool UCombatDeckEditWidget::NativeOnDragOver(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	if (!Cast<UCombatDeckEditDragDropOperation>(InOperation) || !BoundCombatDeck)
	{
		return Super::NativeOnDragOver(InGeometry, InDragDropEvent, InOperation);
	}

	UpdateDragPreview(GetDropInsertIndexFromListGeometry(InGeometry, InDragDropEvent.GetScreenSpacePosition()));
	return true;
}

bool UCombatDeckEditWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	if (!Cast<UCombatDeckEditDragDropOperation>(InOperation) || !BoundCombatDeck)
	{
		return Super::NativeOnDrop(InGeometry, InDragDropEvent, InOperation);
	}

	return CommitDragPreview(GetDropInsertIndexFromListGeometry(InGeometry, InDragDropEvent.GetScreenSpacePosition()));
}

void UCombatDeckEditWidget::AddDropIndicatorToList()
{
	if (!CardListBox || !WidgetTree)
	{
		return;
	}

	USizeBox* IndicatorBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	UBorder* DropIndicator = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
	if (!IndicatorBox || !DropIndicator)
	{
		return;
	}

	if (IsCardListHorizontal())
	{
		IndicatorBox->SetWidthOverride(DropIndicatorHeight);
		IndicatorBox->SetHeightOverride(GetDeckCardHeight());
	}
	else
	{
		IndicatorBox->SetHeightOverride(DropIndicatorHeight);
	}
	DropIndicator->SetBrushColor(DropIndicatorColor);
	IndicatorBox->AddChild(DropIndicator);
	AddWidgetToCardList(IndicatorBox, IsCardListHorizontal() ? FMargin(2.0f, 4.0f, 8.0f, 4.0f) : FMargin(4.0f, 4.0f));
}

void UCombatDeckEditWidget::AddInlineFloatingDragCardToList(const FCombatCardInstance& Card, int32 CardIndex)
{
	if (!CardListBox || !CardSlotClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CombatDeckInput][InlineFloatingDrag] Failed List=%s Class=%s"),
			*GetNameSafe(CardListBox),
			*GetNameSafe(CardSlotClass));
		return;
	}

	UCombatDeckEditCardSlotWidget* FloatingSlot = CreateWidget<UCombatDeckEditCardSlotWidget>(GetOwningPlayer(), CardSlotClass);
	if (!FloatingSlot)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CombatDeckInput][InlineFloatingDrag] Failed CreateWidget Class=%s"),
			*GetNameSafe(CardSlotClass));
		return;
	}

	FloatingSlot->SetCard(this, Card, CardIndex, true);
	FloatingSlot->SetVisibility(ESlateVisibility::HitTestInvisible);
	FloatingSlot->SetRenderOpacity(GamepadFloatingDragOpacity);

	FWidgetTransform FloatingTransform;
	const FVector2D InlineOffset(
		FMath::Clamp(GamepadFloatingDragOffset.X * 0.35f, -8.0f, 8.0f),
		FMath::Clamp(GamepadFloatingDragOffset.Y * 0.25f, -8.0f, 4.0f));
	const float InlineScale = FMath::Clamp(GamepadFloatingDragScale, 1.0f, 1.04f);
	FloatingTransform.Translation = InlineOffset;
	FloatingTransform.Scale = FVector2D(InlineScale, InlineScale);
	FloatingSlot->SetRenderTransform(FloatingTransform);

	AddWidgetToCardList(WrapDeckCardWidget(FloatingSlot),
		IsCardListHorizontal() ? FMargin(0.0f, 0.0f, GetDeckCardSpacing(), 0.0f) : FMargin(0.0f, 2.0f, 0.0f, 2.0f));

	UE_LOG(LogTemp, Warning, TEXT("[CombatDeckInput][InlineFloatingDrag] Added Index=%d Opacity=%.2f Scale=%.2f Offset=(%.1f,%.1f) ConfigOffset=(%.1f,%.1f) Card={%s}"),
		CardIndex,
		GamepadFloatingDragOpacity,
		InlineScale,
		InlineOffset.X,
		InlineOffset.Y,
		GamepadFloatingDragOffset.X,
		GamepadFloatingDragOffset.Y,
		*DescribeCombatDeckCard(Card));
}

int32 UCombatDeckEditWidget::GetPreviewVisualInsertIndex(int32 InsertIndex) const
{
	if (DragSourceIndex == INDEX_NONE)
	{
		return InsertIndex;
	}

	return DragSourceIndex < InsertIndex
		? FMath::Max(0, InsertIndex - 1)
		: InsertIndex;
}

int32 UCombatDeckEditWidget::GetDropInsertIndexFromListGeometry(const FGeometry& InGeometry, const FVector2D& ScreenPosition) const
{
	const TArray<FCombatCardInstance> Cards = BoundCombatDeck ? BoundCombatDeck->GetFullDeckSnapshot() : TArray<FCombatCardInstance>();
	if (Cards.IsEmpty())
	{
		return 0;
	}

	const FGeometry& ListGeometry = CardListBox ? CardListBox->GetCachedGeometry() : InGeometry;
	const FVector2D LocalPosition = ListGeometry.AbsoluteToLocal(ScreenPosition);
	const bool bHorizontal = IsCardListHorizontal();
	const FVector2D ListSize = ListGeometry.GetLocalSize();
	const float SlotExtent = bHorizontal
		? (ListSize.X > 1.0f ? ListSize.X / static_cast<float>(FMath::Max(1, Cards.Num())) : 190.0f)
		: 52.0f;
	const float LocalAxisPosition = bHorizontal ? LocalPosition.X : LocalPosition.Y;
	const int32 VisibleIndex = FMath::Clamp(FMath::RoundToInt(LocalAxisPosition / SlotExtent), 0, FMath::Max(0, Cards.Num() - 1));
	if (bDragPreviewActive && DragSourceIndex != INDEX_NONE && DragSourceIndex <= VisibleIndex)
	{
		return FMath::Clamp(VisibleIndex + 1, 0, Cards.Num());
	}

	return FMath::Clamp(VisibleIndex, 0, Cards.Num());
}

bool UCombatDeckEditWidget::IsCardListHorizontal() const
{
	return Cast<UHorizontalBox>(CardListBox) != nullptr;
}

float UCombatDeckEditWidget::GetDeckCardWidth() const
{
	return StyleDA ? StyleDA->DeckCardWidth : FallbackDeckCardWidth;
}

float UCombatDeckEditWidget::GetDeckCardHeight() const
{
	return StyleDA ? StyleDA->DeckCardHeight : FallbackDeckCardHeight;
}

float UCombatDeckEditWidget::GetDeckCardSpacing() const
{
	return StyleDA ? StyleDA->DeckCardSpacing : FallbackDeckCardSpacing;
}

UWidget* UCombatDeckEditWidget::WrapDeckCardWidget(UWidget* Child)
{
	if (!Child || !bUseFixedTarotCardSize || !WidgetTree)
	{
		return Child;
	}

	USizeBox* SizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	if (!SizeBox)
	{
		return Child;
	}

	SizeBox->SetWidthOverride(GetDeckCardWidth());
	SizeBox->SetHeightOverride(GetDeckCardHeight());
	SizeBox->AddChild(Child);
	return SizeBox;
}

void UCombatDeckEditWidget::AddWidgetToCardList(UWidget* Child, const FMargin& SlotPadding, ESlateSizeRule::Type SizeRule)
{
	if (!CardListBox || !Child)
	{
		return;
	}

	if (UHorizontalBox* HorizontalBox = Cast<UHorizontalBox>(CardListBox))
	{
		if (UHorizontalBoxSlot* AddedHorizontalSlot = HorizontalBox->AddChildToHorizontalBox(Child))
		{
			AddedHorizontalSlot->SetVerticalAlignment(VAlign_Center);
			AddedHorizontalSlot->SetPadding(SlotPadding);
			AddedHorizontalSlot->SetSize(FSlateChildSize(SizeRule));
		}
		return;
	}

	if (UVerticalBox* VerticalBox = Cast<UVerticalBox>(CardListBox))
	{
		if (UVerticalBoxSlot* AddedVerticalSlot = VerticalBox->AddChildToVerticalBox(Child))
		{
			AddedVerticalSlot->SetHorizontalAlignment(HAlign_Fill);
			AddedVerticalSlot->SetPadding(SlotPadding);
			AddedVerticalSlot->SetSize(FSlateChildSize(SizeRule));
		}
		return;
	}

	CardListBox->AddChild(Child);
}
