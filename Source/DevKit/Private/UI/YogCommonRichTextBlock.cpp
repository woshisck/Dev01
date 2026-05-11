#include "UI/YogCommonRichTextBlock.h"
#include "CommonTextBlock.h"
#include "Components/RichTextBlockDecorator.h"

void UYogCommonRichTextBlock::EnsureDecoratorClass(TSubclassOf<URichTextBlockDecorator> DecoratorClass)
{
	if (DecoratorClass && !DecoratorClasses.Contains(DecoratorClass))
	{
		DecoratorClasses.Add(DecoratorClass);
	}
}

void UYogCommonRichTextBlock::SynchronizeProperties()
{
	// Super 会按引擎逻辑设 DefaultTextStyle（基于引擎的 bOverrideDefaultStyle / DefaultTextStyleOverrideClass）
	Super::SynchronizeProperties();

	FTextBlockStyle Merged = GetDefaultTextStyle();
	bool bChanged = false;

	// 1. 应用 FontStyleClass 作为字体基线（覆盖 Super 的结果）
	if (FontStyleClass)
	{
		if (UCommonTextStyle* CDO = Cast<UCommonTextStyle>(FontStyleClass->GetDefaultObject()))
		{
			CDO->ToTextBlockStyle(Merged);
			bChanged = true;
		}
	}

	// 2. 字号覆盖
	if (OverrideFontSize > 0)
	{
		Merged.Font.Size = OverrideFontSize;
		bChanged = true;
	}

	// 3. 颜色覆盖
	if (OverrideColor.A > 0.f)
	{
		Merged.ColorAndOpacity = FSlateColor(OverrideColor);
		bChanged = true;
	}

	if (bChanged)
	{
		SetDefaultTextStyle(Merged);
	}
}
