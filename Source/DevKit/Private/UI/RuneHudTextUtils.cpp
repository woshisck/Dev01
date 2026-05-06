#include "RuneHudTextUtils.h"

#include "Data/LevelInfoPopupDA.h"
#include "Data/RuneDataAsset.h"

namespace
{
	FString StripRichTextMarkup(const FString& Text)
	{
		FString Result;
		Result.Reserve(Text.Len());

		bool bInsideTag = false;
		for (const TCHAR Ch : Text)
		{
			if (Ch == TEXT('<'))
			{
				bInsideTag = true;
				continue;
			}
			if (Ch == TEXT('>') && bInsideTag)
			{
				bInsideTag = false;
				continue;
			}
			if (!bInsideTag)
			{
				Result.AppendChar(Ch);
			}
		}

		return Result;
	}

	FString NormalizeCompactText(FString Text)
	{
		Text = StripRichTextMarkup(Text);
		Text.ReplaceInline(TEXT("\r"), TEXT(" "));
		Text.ReplaceInline(TEXT("\n"), TEXT(" "));
		Text.ReplaceInline(TEXT("\t"), TEXT(" "));
		while (Text.ReplaceInline(TEXT("  "), TEXT(" ")) > 0)
		{
		}
		Text.TrimStartAndEndInline();
		return Text;
	}

	bool IsNoiseSentence(const FString& Text)
	{
		return Text.IsEmpty()
			|| Text.Equals(TEXT("近战武器卡"))
			|| Text.Equals(TEXT("远程武器卡"))
			|| Text.Equals(TEXT("普通卡牌"))
			|| Text.Equals(TEXT("任意攻击"))
			|| Text.Contains(TEXT("测试"))
			|| Text.Contains(TEXT("效果 Tag"))
			|| Text.Contains(TEXT("默认正向连携"))
			|| Text.Contains(TEXT("读取上一张"));
	}

	void SplitSentences(const FString& Text, TArray<FString>& OutSentences)
	{
		FString Current;
		for (const TCHAR Ch : Text)
		{
			const bool bDelimiter = Ch == TEXT('。')
				|| Ch == TEXT('；')
				|| Ch == TEXT(';')
				|| Ch == TEXT('.')
				|| Ch == TEXT('!')
				|| Ch == TEXT('！')
				|| Ch == TEXT('?')
				|| Ch == TEXT('？');
			if (bDelimiter)
			{
				Current.TrimStartAndEndInline();
				if (!Current.IsEmpty())
				{
					OutSentences.Add(Current);
				}
				Current.Reset();
			}
			else
			{
				Current.AppendChar(Ch);
			}
		}

		Current.TrimStartAndEndInline();
		if (!Current.IsEmpty())
		{
			OutSentences.Add(Current);
		}
	}

	FString ShortenCompactText(FString Summary, int32 MaxCharacters)
	{
		Summary = NormalizeCompactText(Summary);
		Summary.ReplaceInline(TEXT("玩家攻击时生成一道"), TEXT("攻击生成"));
		Summary.ReplaceInline(TEXT("玩家攻击时"), TEXT("攻击时"));
		Summary.ReplaceInline(TEXT("攻击命中时，对"), TEXT("命中："));
		Summary.ReplaceInline(TEXT("攻击命中时，"), TEXT("命中："));
		Summary.ReplaceInline(TEXT("攻击命中时,"), TEXT("命中："));
		Summary.ReplaceInline(TEXT("对受伤敌人周围"), TEXT("周围"));
		Summary.ReplaceInline(TEXT(" 内其他敌人造成本次攻击 "), TEXT("内敌人受"));
		Summary.ReplaceInline(TEXT("内其他敌人造成本次攻击"), TEXT("内敌人受"));
		Summary.ReplaceInline(TEXT(" 的溅射伤害"), TEXT("溅射伤害"));
		Summary.ReplaceInline(TEXT("的溅射伤害"), TEXT("溅射伤害"));
		Summary.ReplaceInline(TEXT(" 300cm "), TEXT("300cm"));
		Summary.ReplaceInline(TEXT("该关卡"), TEXT("本关"));
		Summary.ReplaceInline(TEXT("本关敌人"), TEXT("敌人"));
		Summary.TrimStartAndEndInline();

		MaxCharacters = FMath::Max(MaxCharacters, 8);
		if (Summary.Len() > MaxCharacters)
		{
			Summary = Summary.Left(MaxCharacters - 1) + TEXT("…");
		}

		return Summary;
	}

	FText MakeCompactTextFromSource(FString Source, int32 MaxCharacters)
	{
		Source = NormalizeCompactText(Source);

		TArray<FString> Sentences;
		SplitSentences(Source, Sentences);
		for (FString Sentence : Sentences)
		{
			Sentence = NormalizeCompactText(Sentence);
			if (!IsNoiseSentence(Sentence))
			{
				const FString Summary = ShortenCompactText(Sentence, MaxCharacters);
				return Summary.IsEmpty() ? FText::GetEmpty() : FText::FromString(Summary);
			}
		}

		const FString Summary = ShortenCompactText(Source, MaxCharacters);
		return Summary.IsEmpty() ? FText::GetEmpty() : FText::FromString(Summary);
	}
}

namespace RuneHudTextUtils
{
	FText GetRuneHudSummary(const FRuneConfig& Config, int32 MaxCharacters)
	{
		if (!Config.HUDSummaryText.IsEmptyOrWhitespace())
		{
			return Config.HUDSummaryText;
		}

		if (Config.RuneDescription.IsEmptyOrWhitespace())
		{
			return FText::GetEmpty();
		}

		return MakeCompactTextFromSource(Config.RuneDescription.ToString(), MaxCharacters);
	}

	FText GetLevelInfoHudSummary(const ULevelInfoPopupDA& PopupData, int32 /*MaxCharacters*/)
	{
		if (!PopupData.HUDSummaryText.IsEmptyOrWhitespace())
		{
			return PopupData.HUDSummaryText;
		}

		return PopupData.Body;
	}
}
