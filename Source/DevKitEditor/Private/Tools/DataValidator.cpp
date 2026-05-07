#include "Tools/DataValidator.h"

#include "Tools/DataEditorLibrary.h"
#include "Data/RuneDataAsset.h"
#include "Data/EffectDataAsset.h"
#include "Data/RuneEffectFragment.h"

DEFINE_LOG_CATEGORY_STATIC(LogDataValidator, Log, All);

namespace
{
	void EmitError(FDataValidationReport& Out, const FString& Msg)
	{
		++Out.ErrorCount;
		UE_LOG(LogDataValidator, Error, TEXT("%s"), *Msg);
	}

	void EmitWarning(FDataValidationReport& Out, const FString& Msg)
	{
		++Out.WarningCount;
		UE_LOG(LogDataValidator, Warning, TEXT("%s"), *Msg);
	}
}

namespace
{
	// 占位 Tag（来自 Config/Tags/RuneIDs.ini / EffectIDs.ini）— 不能作为正式身份
	bool IsPlaceholderRuneTag(const FGameplayTag& T)
	{
		const FString S = T.ToString();
		return S == TEXT("Rune.ID") || S == TEXT("Rune.ID.Unspecified");
	}
	bool IsPlaceholderEffectTag(const FGameplayTag& T)
	{
		const FString S = T.ToString();
		return S == TEXT("Effect.ID") || S == TEXT("Effect.ID.Unspecified");
	}
}

FDataValidationReport UDataValidator::ValidateAllRuneDAs()
{
	FDataValidationReport Report;
	const TArray<URuneDataAsset*> All = UDataEditorLibrary::GetAllRuneDAs();
	Report.ScannedCount = All.Num();

	TMap<FGameplayTag, URuneDataAsset*> SeenTags;

	for (URuneDataAsset* DA : All)
	{
		if (!DA) continue;

		const FGameplayTag IdTag = DA->GetRuneIdTag();
		const FName Name = DA->GetRuneName();
		const FString AssetName = DA->GetName();

		// 1) RuneIdTag 必填（迁移期 Warning，发布前应清零）
		if (!IdTag.IsValid())
		{
			EmitWarning(Report, FString::Printf(TEXT("[Rune] %s: RuneIdTag 未配置（迁移期允许，正式发布前需补全）"), *AssetName));
		}
		else if (IsPlaceholderRuneTag(IdTag))
		{
			// 1.5) 占位 Tag 不算合法身份
			EmitWarning(Report, FString::Printf(TEXT("[Rune] %s: RuneIdTag 使用占位 Tag %s（应配置具体子 Tag）"),
				*AssetName, *IdTag.ToString()));
		}
		else
		{
			// 2) RuneIdTag 唯一性（仅对非占位 Tag 检查）
			if (URuneDataAsset** Existing = SeenTags.Find(IdTag))
			{
				EmitError(Report, FString::Printf(TEXT("[Rune] RuneIdTag 重复：%s 与 %s 都使用 %s"),
					*AssetName, *(*Existing)->GetName(), *IdTag.ToString()));
			}
			else
			{
				SeenTags.Add(IdTag, DA);
			}
		}

		// 3) RuneName 缺失
		if (Name.IsNone())
		{
			EmitWarning(Report, FString::Printf(TEXT("[Rune] %s: RuneName 为空"), *AssetName));
		}

		// 4) 字段范围
		if (DA->GetGoldCost() < 0)
		{
			EmitError(Report, FString::Printf(TEXT("[Rune] %s: GoldCost < 0 (=%d)"), *AssetName, DA->GetGoldCost()));
		}

		// 5) ChainRole == Producer 但没勾任何方向
		if (DA->GetChainRole() == ERuneChainRole::Producer && DA->GetChainDirections().Num() == 0)
		{
			EmitWarning(Report, FString::Printf(TEXT("[Rune] %s: ChainRole=Producer 但 ChainDirections 为空"), *AssetName));
		}

		// 6) GenericEffects 引用断链
		const TArray<TObjectPtr<UGenericRuneEffectDA>>& Generics = DA->GetGenericEffects();
		for (int32 i = 0; i < Generics.Num(); ++i)
		{
			if (!Generics[i])
			{
				EmitError(Report, FString::Printf(TEXT("[Rune] %s: GenericEffects[%d] 为 null（引用断链）"), *AssetName, i));
			}
		}

		// 7) Shape 异常：cells 为空（理论上至少 1 格）
		if (DA->GetShape().Cells.Num() == 0)
		{
			EmitWarning(Report, FString::Printf(TEXT("[Rune] %s: Shape.Cells 为空（背包格形状未配置）"), *AssetName));
		}
	}

	Report.Summary = FString::Printf(TEXT("RuneDA: %d scanned, %d errors, %d warnings"),
		Report.ScannedCount, Report.ErrorCount, Report.WarningCount);
	UE_LOG(LogDataValidator, Log, TEXT("%s"), *Report.Summary);
	return Report;
}

FDataValidationReport UDataValidator::ValidateAllEffectDAs()
{
	FDataValidationReport Report;
	const TArray<UEffectDataAsset*> All = UDataEditorLibrary::GetAllEffectDAs();
	Report.ScannedCount = All.Num();

	TMap<FGameplayTag, UEffectDataAsset*> SeenTags;

	for (UEffectDataAsset* DA : All)
	{
		if (!DA) continue;
		const FString AssetName = DA->GetName();
		const FGameplayTag IdTag = DA->GetEffectIdTag();

		// 1) EffectIdTag 唯一性（无 Tag 允许，匿名效果合法）
		if (IdTag.IsValid())
		{
			if (IsPlaceholderEffectTag(IdTag))
			{
				EmitWarning(Report, FString::Printf(TEXT("[Effect] %s: EffectTag 使用占位 Tag %s（应配置具体子 Tag）"),
					*AssetName, *IdTag.ToString()));
			}
			else if (UEffectDataAsset** Existing = SeenTags.Find(IdTag))
			{
				EmitError(Report, FString::Printf(TEXT("[Effect] EffectTag 重复：%s 与 %s 都使用 %s"),
					*AssetName, *(*Existing)->GetName(), *IdTag.ToString()));
			}
			else
			{
				SeenTags.Add(IdTag, DA);
			}
		}

		// 2) Duration 合法性
		if (DA->GetDurationType() == ERuneDurationType::Duration && DA->GetDuration() <= 0.f)
		{
			EmitError(Report, FString::Printf(TEXT("[Effect] %s: DurationType=Duration 但 Duration <= 0 (=%.2f)"),
				*AssetName, DA->GetDuration()));
		}

		// 3) Period 合法性
		if (DA->GetPeriod() < 0.f)
		{
			EmitError(Report, FString::Printf(TEXT("[Effect] %s: Period < 0 (=%.2f)"), *AssetName, DA->GetPeriod()));
		}

		// 4) Fragment 引用断链
		const TArray<TObjectPtr<URuneEffectFragment>>& Fragments = DA->GetFragments();
		for (int32 i = 0; i < Fragments.Num(); ++i)
		{
			if (!Fragments[i])
			{
				EmitError(Report, FString::Printf(TEXT("[Effect] %s: Effects[%d] 为 null（引用断链）"), *AssetName, i));
			}
		}

		// 5) Stack 配置自洽性
		if (DA->GetUniqueType() != ERuneUniqueType::NonUnique
			&& DA->GetStackType() == ERuneStackType::Stack
			&& DA->GetMaxStack() < 1)
		{
			EmitError(Report, FString::Printf(TEXT("[Effect] %s: StackType=Stack 但 MaxStack < 1 (=%d)"),
				*AssetName, DA->GetMaxStack()));
		}
	}

	Report.Summary = FString::Printf(TEXT("EffectDA: %d scanned, %d errors, %d warnings"),
		Report.ScannedCount, Report.ErrorCount, Report.WarningCount);
	UE_LOG(LogDataValidator, Log, TEXT("%s"), *Report.Summary);
	return Report;
}

FDataValidationReport UDataValidator::ValidateAll()
{
	const FDataValidationReport R1 = ValidateAllRuneDAs();
	const FDataValidationReport R2 = ValidateAllEffectDAs();

	FDataValidationReport Combined;
	Combined.ScannedCount = R1.ScannedCount + R2.ScannedCount;
	Combined.ErrorCount   = R1.ErrorCount   + R2.ErrorCount;
	Combined.WarningCount = R1.WarningCount + R2.WarningCount;
	Combined.Summary = FString::Printf(TEXT("All: %d scanned (%d Rune + %d Effect), %d errors, %d warnings"),
		Combined.ScannedCount, R1.ScannedCount, R2.ScannedCount,
		Combined.ErrorCount, Combined.WarningCount);
	UE_LOG(LogDataValidator, Log, TEXT("%s"), *Combined.Summary);
	return Combined;
}
