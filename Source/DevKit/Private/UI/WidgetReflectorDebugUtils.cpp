#include "UI/WidgetReflectorDebugUtils.h"

#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/PanelWidget.h"
#include "Components/Widget.h"

namespace YogWidgetReflectorDebug
{
bool ShouldMakeWidgetsPickable()
{
#if WITH_EDITOR
	return true;
#else
	return false;
#endif
}

ESlateVisibility GetInspectableVisibility(ESlateVisibility Visibility)
{
	if (!ShouldMakeWidgetsPickable())
	{
		return Visibility;
	}

	return Visibility == ESlateVisibility::Collapsed || Visibility == ESlateVisibility::Hidden
		? Visibility
		: ESlateVisibility::Visible;
}

void ApplyToWidgetTree(UWidget* Widget)
{
	if (!ShouldMakeWidgetsPickable() || !Widget)
	{
		return;
	}

	Widget->SetVisibility(GetInspectableVisibility(Widget->GetVisibility()));

	if (UUserWidget* UserWidget = Cast<UUserWidget>(Widget))
	{
		if (UWidgetTree* NestedWidgetTree = UserWidget->WidgetTree)
		{
			ApplyToWidgetTree(NestedWidgetTree->RootWidget);
		}
	}

	if (UPanelWidget* PanelWidget = Cast<UPanelWidget>(Widget))
	{
		const int32 ChildCount = PanelWidget->GetChildrenCount();
		for (int32 ChildIndex = 0; ChildIndex < ChildCount; ++ChildIndex)
		{
			ApplyToWidgetTree(PanelWidget->GetChildAt(ChildIndex));
		}
	}
}
}
