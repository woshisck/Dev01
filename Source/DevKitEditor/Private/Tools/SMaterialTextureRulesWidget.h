#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class STextBlock;
class UTexture2D;
class UYogMaterialTextureNamingConvention;

class SMaterialTextureRulesWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SMaterialTextureRulesWidget) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	FReply WriteDefaultTextureNamingConventionAsset();
	FReply OpenDefaultTextureNamingConventionAsset();
	FReply CheckSelectedTextureNaming();
	void SetStatus(const FText& InStatus) const;
	FText GetUsageText() const;

	TArray<UTexture2D*> CollectSelectedTextures() const;
	UYogMaterialTextureNamingConvention* LoadOrCreateDefaultConvention(bool bFillDefaultRules) const;

	TSharedPtr<STextBlock> StatusTextBlock;
};
