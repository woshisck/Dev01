#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Text/STextBlock.h"

class SCharacterBalanceWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SCharacterBalanceWidget) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs)
	{
		ChildSlot
		[
			SNew(STextBlock)
			.Text(NSLOCTEXT("DevKitEditor", "CharacterBalanceWidgetUnavailable", "Character Balance editor is unavailable in this checkout."))
		];
	}
};
