#include "UI/GenericEffectListWidget.h"
#include "Data/GenericRuneEffectDA.h"
#include "Components/VerticalBox.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "CommonRichTextBlock.h"

// ============================================================
//  UGenericEffectEntryWidget
// ============================================================

void UGenericEffectEntryWidget::SetData(UGenericRuneEffectDA* Effect)
{
	if (!Effect) return;

	if (EffectName)
	{
		EffectName->SetText(Effect->DisplayName);
	}

	if (EffectDesc)
	{
		EffectDesc->SetText(Effect->Description);
	}

	if (EffectIcon)
	{
		if (Effect->Icon)
		{
			EffectIcon->SetBrushFromTexture(Effect->Icon, false);
			EffectIcon->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		}
		else
		{
			EffectIcon->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

// ============================================================
//  UGenericEffectListWidget
// ============================================================

void UGenericEffectListWidget::SetEffects(const TArray<UGenericRuneEffectDA*>& Effects)
{
	if (!EntriesBox) return;

	EntriesBox->ClearChildren();

	if (Effects.Num() == 0 || !EntryClass)
	{
		SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	for (UGenericRuneEffectDA* Effect : Effects)
	{
		if (!Effect) continue;
		UGenericEffectEntryWidget* Entry = CreateWidget<UGenericEffectEntryWidget>(this, EntryClass);
		if (!Entry) continue;
		Entry->SetData(Effect);
		EntriesBox->AddChild(Entry);
	}
}
