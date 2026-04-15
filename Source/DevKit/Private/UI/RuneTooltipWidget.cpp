#include "UI/RuneTooltipWidget.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"

void URuneTooltipWidget::ShowRuneInfo(const FRuneInstance& Rune)
{
    if (TooltipName)
        TooltipName->SetText(FText::FromName(Rune.RuneConfig.RuneName));

    if (TooltipDesc)
        TooltipDesc->SetText(Rune.RuneConfig.RuneDescription);

    if (TooltipType)
    {
        FText TypeText;
        switch (Rune.RuneConfig.RuneType)
        {
        case ERuneType::Buff:   TypeText = NSLOCTEXT("Tooltip", "Buff",   "增益"); break;
        case ERuneType::Debuff: TypeText = NSLOCTEXT("Tooltip", "Debuff", "减益"); break;
        default:                TypeText = NSLOCTEXT("Tooltip", "None",   "");      break;
        }
        TooltipType->SetText(TypeText);
    }

    if (TooltipIcon)
    {
        if (Rune.RuneConfig.RuneIcon)
        {
            TooltipIcon->SetBrushFromTexture(Rune.RuneConfig.RuneIcon, false);
            TooltipIcon->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
        }
        else
        {
            TooltipIcon->SetVisibility(ESlateVisibility::Collapsed);
        }
    }

    SetVisibility(ESlateVisibility::SelfHitTestInvisible);
    OnTooltipShown(Rune);
}

void URuneTooltipWidget::HideTooltip()
{
    SetVisibility(ESlateVisibility::Collapsed);
    OnTooltipHidden();
}
