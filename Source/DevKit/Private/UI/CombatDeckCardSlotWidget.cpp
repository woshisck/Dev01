#include "UI/CombatDeckCardSlotWidget.h"

#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "UI/YogCommonRichTextBlock.h"

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

void UCombatDeckCardSlotWidget::SetCard(const FCombatCardInstance& InCard, bool bIsNextCard)
{
	SetVisibility(ESlateVisibility::Visible);

	if (CardFrame)
	{
		CardFrame->SetBrushColor(bIsNextCard ? NextCardFrameColor : NormalCardFrameColor);
	}

	if (CardIcon)
	{
		UTexture2D* RuneIcon = InCard.SourceData ? InCard.SourceData->RuneInfo.RuneConfig.RuneIcon : nullptr;
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
