#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Text/STextBlock.h"

class SDataEditorWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SDataEditorWidget) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs)
	{
		ChildSlot
		[
			SNew(STextBlock)
			.Text(NSLOCTEXT("DevKitEditor", "RuneBalanceWidgetUnavailable", "Rune Balance editor is unavailable in this checkout. Use Rune Editor for current rune authoring."))
		];
	}
};
