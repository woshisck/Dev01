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
	PreviewCard.Config.RequiredAction = ECardRequiredAction::Light;
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

	if (CardFrame)
	{
		CardFrame->SetBrushColor(bIsNextCard ? NextCardFrameColor : NormalCardFrameColor);
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
		SetTextIfSupported(StateText, bIsNextCard ? FText::FromString(TEXT("NEXT")) : FText::GetEmpty());
	}
}

void UCombatDeckCardSlotWidget::ClearSlot()
{
	SetVisibility(ESlateVisibility::Collapsed);
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
		return FText::FromString(TEXT("Light"));
	case ECardRequiredAction::Heavy:
		return FText::FromString(TEXT("Heavy"));
	case ECardRequiredAction::Any:
	default:
		return FText::FromString(TEXT("Any"));
	}
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
	const float FoldAlpha = Alpha < 0.5f
		? Alpha * 2.0f
		: (1.0f - Alpha) * 2.0f;
	const float SmoothFoldAlpha = FMath::InterpEaseInOut(0.0f, 1.0f, FoldAlpha, 2.0f);
	const float ScaleX = FMath::Lerp(1.0f, FMath::Clamp(UseFlipMinScaleX, 0.0f, 1.0f), SmoothFoldAlpha);
	const float ScaleY = FMath::Lerp(1.0f, FMath::Max(1.0f, UseFlipPeakScaleY), SmoothFoldAlpha);

	SetRenderScale(FVector2D(ScaleX, ScaleY));
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
		return FText::FromString(TEXT("Link"));
	case ECombatCardType::Finisher:
		return FText::FromString(TEXT("Finisher"));
	case ECombatCardType::Passive:
		return FText::FromString(TEXT("Passive"));
	case ECombatCardType::Attack:
	default:
		return FText::FromString(TEXT("Attack"));
	}
}
