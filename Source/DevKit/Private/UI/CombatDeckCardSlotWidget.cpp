#include "UI/CombatDeckCardSlotWidget.h"

#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "UI/YogCommonRichTextBlock.h"

void UCombatDeckCardSlotWidget::NativeConstruct()
{
	Super::NativeConstruct();

	SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	ResetUseFlipTransform();
}

void UCombatDeckCardSlotWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

	if (!IsDesignTime())
	{
		return;
	}

	FCombatCardInstance PreviewCard;
	PreviewCard.Config.bIsCombatCard = true;
	PreviewCard.Config.DisplayName = FText::FromString(TEXT("Card"));
	PreviewCard.Config.RequiredAction = ECardRequiredAction::Any;
	PreviewCard.Config.CardType = ECombatCardType::Attack;
	SetCard(PreviewCard, true);
}

void UCombatDeckCardSlotWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!bUseFlipAnimating)
	{
		return;
	}

	const float SafeDuration = FMath::Max(0.01f, UseFlipDuration);
	UseFlipElapsed = FMath::Min(UseFlipElapsed + FMath::Max(0.0f, InDeltaTime), SafeDuration);
	ApplyUseFlipTransform(UseFlipElapsed / SafeDuration);

	if (UseFlipElapsed >= SafeDuration)
	{
		bUseFlipAnimating = false;
		ResetUseFlipTransform();
	}
}

void UCombatDeckCardSlotWidget::SetCard(const FCombatCardInstance& InCard, bool bIsNextCard)
{
	SetVisibility(ESlateVisibility::Visible);
	SetRenderOpacity(InCard.bTemporarilyLocked ? LockedCardOpacity : 1.0f);

	if (CardFrame)
	{
		const FLinearColor FrameColor = InCard.bTemporarilyLocked
			? LockedCardFrameColor
			: (bIsNextCard ? NextCardFrameColor : NormalCardFrameColor);
		CardFrame->SetBrushColor(FrameColor);
	}

	if (CardIcon)
	{
		UTexture2D* RuneIcon = InCard.SourceData ? InCard.SourceData->GetRuneIcon() : nullptr;
		CardIcon->SetVisibility(RuneIcon ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		if (RuneIcon)
		{
			CardIcon->SetBrushFromTexture(RuneIcon);
		}
	}

	if (CardNameText)
	{
		SetTextIfSupported(CardNameText, GetCardDisplayName(InCard));
	}

	if (ActionText)
	{
		SetTextIfSupported(ActionText, GetActionText(InCard.Config.RequiredAction));
	}

	if (TypeText)
	{
		SetTextIfSupported(TypeText, GetTypeText(InCard.Config.CardType));
	}

	if (StateText)
	{
		SetTextIfSupported(StateText, GetStateText(InCard, bIsNextCard));
	}
}

void UCombatDeckCardSlotWidget::ClearSlot()
{
	SetVisibility(ESlateVisibility::Collapsed);
	SetRenderOpacity(1.0f);
	bUseFlipAnimating = false;
	ResetUseFlipTransform();

	if (CardFrame)
	{
		CardFrame->SetBrushColor(EmptyCardFrameColor);
	}

	if (CardIcon)
	{
		CardIcon->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (CardNameText)
	{
		SetTextIfSupported(CardNameText, FText::GetEmpty());
	}

	if (ActionText)
	{
		SetTextIfSupported(ActionText, FText::GetEmpty());
	}

	if (TypeText)
	{
		SetTextIfSupported(TypeText, FText::GetEmpty());
	}

	if (StateText)
	{
		SetTextIfSupported(StateText, FText::GetEmpty());
	}
}

void UCombatDeckCardSlotWidget::PlayUseFlipAnimation()
{
	if (IsDesignTime())
	{
		return;
	}

	SetVisibility(ESlateVisibility::Visible);
	SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	UseFlipElapsed = 0.0f;
	bUseFlipAnimating = true;
	ApplyUseFlipTransform(0.0f);
}

FText UCombatDeckCardSlotWidget::GetCardDisplayName(const FCombatCardInstance& Card)
{
	if (!Card.Config.DisplayName.IsEmpty())
	{
		return Card.Config.DisplayName;
	}

	if (Card.SourceData)
	{
		return FText::FromString(Card.SourceData->GetName());
	}

	return FText::FromString(TEXT("Card"));
}

FText UCombatDeckCardSlotWidget::GetActionText(ECardRequiredAction RequiredAction)
{
	switch (RequiredAction)
	{
	case ECardRequiredAction::Light:
	case ECardRequiredAction::Heavy:
		return FText::FromString(TEXT("\u653b\u51fb"));
	case ECardRequiredAction::Any:
	default:
		return FText::FromString(TEXT("\u89e6\u53d1"));
	}
}

FText UCombatDeckCardSlotWidget::GetStateText(const FCombatCardInstance& Card, bool bIsNextCard)
{
	if (Card.bTemporarilyLocked)
	{
		return FText::Format(
			FText::FromString(TEXT("LOCK {0}/{1}")),
			FText::AsNumber(Card.TemporaryUnlockCurrentCompletedBattles),
			FText::AsNumber(Card.TemporaryUnlockRequiredCompletedBattles));
	}

	return bIsNextCard ? FText::FromString(TEXT("\u5f53\u524d")) : FText::GetEmpty();
}

void UCombatDeckCardSlotWidget::SetTextIfSupported(UWidget* Widget, const FText& Text)
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

void UCombatDeckCardSlotWidget::ApplyUseFlipTransform(float NormalizedAlpha)
{
	const float Alpha = FMath::Clamp(NormalizedAlpha, 0.0f, 1.0f);
	const float PulseAlpha = FMath::Sin(Alpha * UE_PI);
	const float SmoothPulseAlpha = FMath::InterpEaseInOut(0.0f, 1.0f, PulseAlpha, 2.0f);
	const float PulseScale = FMath::Lerp(1.0f, FMath::Max(1.0f, UseFlipPeakScaleY), SmoothPulseAlpha);

	SetRenderScale(FVector2D(PulseScale, PulseScale));
}

void UCombatDeckCardSlotWidget::ResetUseFlipTransform()
{
	SetRenderScale(FVector2D(1.0f, 1.0f));
}

FText UCombatDeckCardSlotWidget::GetTypeText(ECombatCardType CardType)
{
	switch (CardType)
	{
	case ECombatCardType::Link:
		return FText::FromString(TEXT("\u8fde\u643a"));
	case ECombatCardType::Finisher:
		return FText::FromString(TEXT("\u7ec8\u7ed3"));
	case ECombatCardType::Passive:
		return FText::FromString(TEXT("\u88ab\u52a8"));
	case ECombatCardType::Normal:
		return FText::FromString(TEXT("\u666e\u901a"));
	case ECombatCardType::Attack:
	default:
		return FText::FromString(TEXT("\u542f\u52a8"));
	}
}
