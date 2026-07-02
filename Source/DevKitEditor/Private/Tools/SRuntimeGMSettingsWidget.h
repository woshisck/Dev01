#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class IDetailsView;
class STextBlock;

class SRuntimeGMSettingsWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SRuntimeGMSettingsWidget) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	FReply SaveSettings();
	FReply RunPlayablePerfPackage();
	FReply OpenPackageScriptFolder();
	FText GetPackageScriptText() const;
	FString GetPackageBatchPath() const;
	void SetStatus(const FText& InStatus) const;

	TSharedPtr<IDetailsView> SettingsDetailsView;
	TSharedPtr<STextBlock> StatusTextBlock;
};
