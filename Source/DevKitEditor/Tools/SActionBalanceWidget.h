#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Text/STextBlock.h"

class SActionBalanceWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SActionBalanceWidget) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs)
	{
		ChildSlot
		[
			SNew(STextBlock)
			.Text(NSLOCTEXT("DevKitEditor", "ActionBalanceWidgetUnavailable", "Action Balance editor is unavailable in this checkout."))
		];
	}
};
