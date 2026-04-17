#include "UI/RuneInfoCardWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"

void URuneInfoCardWidget::ShowRune(const FRuneInstance& Rune)
{
    SetVisibility(ESlateVisibility::SelfHitTestInvisible);

    if (CardName)
        CardName->SetText(FText::FromName(Rune.RuneConfig.RuneName));

    if (CardDesc)
        CardDesc->SetText(Rune.RuneConfig.RuneDescription);

    if (CardIcon)
    {
        if (Rune.RuneConfig.RuneIcon)
        {
            CardIcon->SetBrushFromTexture(Rune.RuneConfig.RuneIcon, false);
            CardIcon->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
        }
        else
        {
            CardIcon->SetVisibility(ESlateVisibility::Collapsed);
        }
    }

    if (CardUpgrade)
    {
        if (Rune.UpgradeLevel > 0)
        {
            CardUpgrade->SetText(FText::Format(
                NSLOCTEXT("RuneInfoCard", "LvText", "Lv.{0}"),
                FText::AsNumber(Rune.UpgradeLevel + 1)));
            CardUpgrade->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
        }
        else
        {
            CardUpgrade->SetVisibility(ESlateVisibility::Collapsed);
        }
    }
}

void URuneInfoCardWidget::HideCard()
{
    SetVisibility(ESlateVisibility::Collapsed);
}
