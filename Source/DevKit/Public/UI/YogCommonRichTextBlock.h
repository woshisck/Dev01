#pragma once

#include "CoreMinimal.h"
#include "CommonRichTextBlock.h"
#include "YogCommonRichTextBlock.generated.h"

class UCommonTextStyle;

/**
 * 项目自定义的 CommonRichTextBlock：
 * 解决引擎原版 DefaultTextStyleOverrideClass 必须勾 bOverrideDefaultStyle 才能编辑、
 * 且勾了之后字号/颜色又被 inline override 接管的痛点。
 *
 * 用法（引擎自带的 Default Text Style Override Class / Override Default Style 全部忽略，留 None / 不勾）：
 *   - FontStyleClass         = BP_InfoPopupTextStyle    （提供中文字体基线）
 *   - OverrideFontSize       = 14                       （>0 时覆盖字号；0 = 用 Class 默认）
 *   - OverrideColor          = (R=0.93,G=0.93,B=0.93,A=1)（A>0 时覆盖颜色；A=0 = 用 Class 默认）
 *
 * 优先级：FontStyleClass 基线 → OverrideFontSize → OverrideColor
 */
UCLASS(meta = (DisplayName = "Yog Common Rich Text Block"))
class DEVKIT_API UYogCommonRichTextBlock : public UCommonRichTextBlock
{
	GENERATED_BODY()

public:
	/** 文字样式类（提供字体基线）。代替引擎被锁住的 DefaultTextStyleOverrideClass。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Yog Style Override")
	TSubclassOf<UCommonTextStyle> FontStyleClass;

	/** 字号覆盖；0 = 不覆盖（用 FontStyleClass 默认字号） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Yog Style Override", meta = (ClampMin = "0"))
	int32 OverrideFontSize = 0;

	/** 颜色覆盖；Alpha = 0 视为不覆盖（用 FontStyleClass 默认颜色） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Yog Style Override")
	FLinearColor OverrideColor = FLinearColor(1.f, 1.f, 1.f, 0.f);

protected:
	virtual void SynchronizeProperties() override;
};
