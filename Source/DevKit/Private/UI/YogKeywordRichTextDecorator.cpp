#include "UI/YogKeywordRichTextDecorator.h"
#include "Components/RichTextBlock.h"
#include "Framework/Text/SlateTextRun.h"
#include "Framework/Text/ITextDecorator.h"
#include "Styling/SlateStyle.h"
#include "Styling/SlateTypes.h"

class FYogKeywordTextDecorator : public ITextDecorator
{
public:
    explicit FYogKeywordTextDecorator(UYogKeywordRichTextDecorator* InOwner)
        : Owner(InOwner)
    {
    }

    virtual bool Supports(const FTextRunParseResults& RunParseResult, const FString& /*Text*/) const override
    {
        return RunParseResult.Name == TEXT("key");
    }

    virtual TSharedRef<ISlateRun> Create(
        const TSharedRef<class FTextLayout>& /*TextLayout*/,
        const FTextRunParseResults& RunParseResult,
        const FString& OriginalText,
        const TSharedRef<FString>& InOutModelText,
        const ISlateStyle* Style) override
    {
        FRunInfo RunInfo(RunParseResult.Name);

        // 提取标签内文本：<key>击退</key> → "击退"
        const int32 ContentBegin = RunParseResult.ContentRange.BeginIndex;
        const int32 ContentLen   = RunParseResult.ContentRange.EndIndex - ContentBegin;
        const FString Content    = OriginalText.Mid(ContentBegin, ContentLen);
        *InOutModelText += Content;

        // 派生基础 TextStyle：优先用 RichTextBlock 自身的样式集（保留项目字体），
        // 找不到时 fallback 到 FTextBlockStyle 的全局默认（避免使用 FCoreStyle 的 NormalText
        // 把项目中文字体重置成引擎默认）
        FTextBlockStyle TextStyle = FTextBlockStyle::GetDefault();
        if (Style)
        {
            static const FName CandidateStyleNames[] =
            {
                FName(TEXT("NormalText")),
                FName(TEXT("Default")),
                FName(TEXT("RichTextBlock.Text"))
            };

            for (const FName& StyleName : CandidateStyleNames)
            {
                if (Style->HasWidgetStyle<FTextBlockStyle>(StyleName))
                {
                    TextStyle = Style->GetWidgetStyle<FTextBlockStyle>(StyleName);
                    break;
                }
            }
        }

        if (Owner.IsValid())
        {
            TextStyle.ColorAndOpacity = FSlateColor(Owner->KeywordColor);
            if (Owner->bBold)
            {
                TextStyle.Font.TypefaceFontName = FName(TEXT("Bold"));
            }
            if (Owner->FontSizeOverride > 0)
            {
                TextStyle.Font.Size = Owner->FontSizeOverride;
            }
        }

        FTextRange ModelRange;
        ModelRange.BeginIndex = InOutModelText->Len() - Content.Len();
        ModelRange.EndIndex   = InOutModelText->Len();
        return FSlateTextRun::Create(RunInfo, InOutModelText, TextStyle, ModelRange);
    }

private:
    TWeakObjectPtr<UYogKeywordRichTextDecorator> Owner;
};

TSharedPtr<ITextDecorator> UYogKeywordRichTextDecorator::CreateDecorator(URichTextBlock* /*InOwner*/)
{
    return MakeShared<FYogKeywordTextDecorator>(this);
}
