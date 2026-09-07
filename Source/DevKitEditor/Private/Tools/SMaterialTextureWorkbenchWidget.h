#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class SBox;
class STextBlock;
class SWidgetSwitcher;

class SMaterialTextureWorkbenchWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SMaterialTextureWorkbenchWidget) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	enum class EPage : uint8
	{
		RVTLibrary,
		MaterialCompliance,
		TextureAudit,
		TextureCollection
	};

	TSharedRef<SWidget> BuildNavigation();
	TSharedRef<SWidget> BuildNavigationButton(EPage Page, const FText& Title, const FText& Summary);
	TSharedRef<SWidget> BuildHelpPanel();
	FReply SelectPage(EPage Page);
	FText GetPageTitle() const;
	FText GetPageSummary() const;
	FText GetPageHelpRichText() const;
	FSlateColor GetNavigationColor(EPage Page) const;

	EPage ActivePage = EPage::RVTLibrary;
	TSharedPtr<SWidgetSwitcher> ToolSwitcher;
};
