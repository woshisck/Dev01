#pragma once

#include "CoreMinimal.h"
#include "Components/RichTextBlockDecorator.h"
#include "Styling/SlateTypes.h"
#include "YogKeywordRichTextDecorator.generated.h"

/**
 * 关键词高亮 RichText Decorator
 *
 * 在 DA 的 RuneDescription / 通用文案里写 `<key>击退</key>`，自动渲染为加粗 + 着色。
 *
 * 用法（WBP）：
 *   1. CardDesc / CardEffect 等 RichText 控件（CommonRichTextBlock）
 *   2. Decorator Classes [+] = BP_KeywordDecorator
 *   3. BP_KeywordDecorator 父类选 UYogKeywordRichTextDecorator
 *   4. Class Defaults 配 KeywordColor / bBold / FontSizeOverride
 *
 * DA 编辑示例：
 *   "冲刺后攻击造成 10 点伤害，并附加 <key>击退</key>"
 *   "造成 5 点 <key>燃烧</key> 伤害，并 <key>击退</key> 目标"
 */
UCLASS(Blueprintable)
class DEVKIT_API UYogKeywordRichTextDecorator : public URichTextBlockDecorator
{
    GENERATED_BODY()

public:
    /** 关键词颜色（默认金黄 #FFD966） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Keyword")
    FLinearColor KeywordColor = FLinearColor(1.f, 0.85f, 0.4f, 1.f);

    /** 是否将关键词字体设为 Bold（要求字体资产含 Bold typeface，否则 fallback 到 Regular） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Keyword")
    bool bBold = true;

    /** 字号覆盖（0 = 不覆盖，沿用 RichText 默认字号） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Keyword", meta = (ClampMin = "0"))
    int32 FontSizeOverride = 0;

    virtual TSharedPtr<ITextDecorator> CreateDecorator(URichTextBlock* InOwner) override;
};
