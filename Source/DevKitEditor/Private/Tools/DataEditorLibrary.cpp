#include "Tools/DataEditorLibrary.h"

#include "Data/CharacterData.h"
#include "Data/EffectDataAsset.h"
#include "Data/MontageAttackDataAsset.h"
#include "Data/MontageConfigDA.h"
#include "Data/MusketActionTuningDataAsset.h"
#include "FlowAsset.h"
#include "AssetRegistry/AssetRegistryHelpers.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Engine/DataAsset.h"
#include "ScopedTransaction.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/DateTime.h"
#include "HAL/PlatformFileManager.h"
#include "GameplayTagsManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogDataEditor, Log, All);

#define LOCTEXT_NAMESPACE "DataEditorLibrary"

namespace
{
	template <typename T>
	TArray<T*> CollectAssetsOfClass()
	{
		TArray<T*> Out;
		IAssetRegistry& AR = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();
		if (AR.IsLoadingAssets())
		{
			AR.SearchAllAssets(true);
		}

		TArray<FAssetData> Assets;
		const UClass* TargetClass = T::StaticClass();
		const UClass* QueryClass = TargetClass->IsChildOf(UDataAsset::StaticClass())
			? UDataAsset::StaticClass()
			: TargetClass;
		AR.GetAssetsByClass(QueryClass->GetClassPathName(), Assets, /*bSearchSubClasses=*/ true);
		Out.Reserve(Assets.Num());
		for (const FAssetData& A : Assets)
		{
			const UClass* NativeClass = UAssetRegistryHelpers::FindAssetNativeClass(A);
			if (!A.IsInstanceOf(TargetClass, EResolveClass::Yes)
				&& (!NativeClass || !NativeClass->IsChildOf(T::StaticClass())))
			{
				continue;
			}

			if (T* Loaded = Cast<T>(A.GetAsset()))
			{
				Out.Add(Loaded);
			}
		}
		return Out;
	}

	FString RarityToString(ERuneRarity R)
	{
		switch (R)
		{
		case ERuneRarity::Common:    return TEXT("Common");
		case ERuneRarity::Rare:      return TEXT("Rare");
		case ERuneRarity::Epic:      return TEXT("Epic");
		case ERuneRarity::Legendary: return TEXT("Legendary");
		}
		return TEXT("Unknown");
	}

	FString TriggerTypeToString(ERuneTriggerType T)
	{
		switch (T)
		{
		case ERuneTriggerType::Passive:          return TEXT("Passive");
		case ERuneTriggerType::OnAttackHit:      return TEXT("OnAttackHit");
		case ERuneTriggerType::OnDash:           return TEXT("OnDash");
		case ERuneTriggerType::OnKill:           return TEXT("OnKill");
		case ERuneTriggerType::OnCritHit:        return TEXT("OnCritHit");
		case ERuneTriggerType::OnDamageReceived: return TEXT("OnDamageReceived");
		}
		return TEXT("Unknown");
	}

	FString DurationTypeToString(ERuneDurationType T)
	{
		switch (T)
		{
		case ERuneDurationType::Instant:  return TEXT("Instant");
		case ERuneDurationType::Infinite: return TEXT("Infinite");
		case ERuneDurationType::Duration: return TEXT("Duration");
		}
		return TEXT("Unknown");
	}

	FString CsvEscape(const FString& In)
	{
		FString S = In;
		const bool bNeedsQuotes = S.Contains(TEXT(",")) || S.Contains(TEXT("\"")) || S.Contains(TEXT("\n"));
		S.ReplaceInline(TEXT("\""), TEXT("\"\""));
		return bNeedsQuotes ? FString::Printf(TEXT("\"%s\""), *S) : S;
	}
}

TArray<URuneDataAsset*> UDataEditorLibrary::GetAllRuneDAs()
{
	return CollectAssetsOfClass<URuneDataAsset>();
}

TArray<UEffectDataAsset*> UDataEditorLibrary::GetAllEffectDAs()
{
	return CollectAssetsOfClass<UEffectDataAsset>();
}

TArray<UCharacterData*> UDataEditorLibrary::GetAllCharacterDAs()
{
	return CollectAssetsOfClass<UCharacterData>();
}

TArray<UMontageConfigDA*> UDataEditorLibrary::GetAllMontageConfigDAs()
{
	return CollectAssetsOfClass<UMontageConfigDA>();
}

TArray<UMontageAttackDataAsset*> UDataEditorLibrary::GetAllMontageAttackDataDAs()
{
	return CollectAssetsOfClass<UMontageAttackDataAsset>();
}

TArray<UMusketActionTuningDataAsset*> UDataEditorLibrary::GetAllMusketActionTuningDAs()
{
	return CollectAssetsOfClass<UMusketActionTuningDataAsset>();
}

void UDataEditorLibrary::BatchSetRuneGoldCost(const TArray<URuneDataAsset*>& Targets, int32 NewGoldCost)
{
	const int32 Clamped = FMath::Max(0, NewGoldCost);
	if (Clamped != NewGoldCost)
	{
		UE_LOG(LogDataEditor, Warning, TEXT("BatchSetRuneGoldCost: input %d clamped to %d (must be >= 0)"), NewGoldCost, Clamped);
	}
	const FScopedTransaction Tx(LOCTEXT("BatchSetRuneGoldCost", "Batch Set Rune GoldCost"));
	for (URuneDataAsset* DA : Targets)
	{
		if (!DA) continue;
		DA->Modify();
		DA->RuneInfo.RuneConfig.GoldCost = Clamped;
		DA->MarkPackageDirty();
	}
	UE_LOG(LogDataEditor, Log, TEXT("BatchSetRuneGoldCost: %d assets set to %d"), Targets.Num(), Clamped);
}

void UDataEditorLibrary::BatchSetRuneRarity(const TArray<URuneDataAsset*>& Targets, ERuneRarity NewRarity)
{
	const FScopedTransaction Tx(LOCTEXT("BatchSetRuneRarity", "Batch Set Rune Rarity"));
	for (URuneDataAsset* DA : Targets)
	{
		if (!DA) continue;
		DA->Modify();
		DA->RuneInfo.RuneConfig.Rarity = NewRarity;
		DA->MarkPackageDirty();
	}
	UE_LOG(LogDataEditor, Log, TEXT("BatchSetRuneRarity: %d assets set to %s"), Targets.Num(), *RarityToString(NewRarity));
}

void UDataEditorLibrary::BatchSetRuneTriggerType(const TArray<URuneDataAsset*>& Targets, ERuneTriggerType NewType)
{
	const FScopedTransaction Tx(LOCTEXT("BatchSetRuneTriggerType", "Batch Set Rune TriggerType"));
	for (URuneDataAsset* DA : Targets)
	{
		if (!DA) continue;
		DA->Modify();
		DA->RuneInfo.RuneConfig.TriggerType = NewType;
		DA->MarkPackageDirty();
	}
	UE_LOG(LogDataEditor, Log, TEXT("BatchSetRuneTriggerType: %d assets set to %s"), Targets.Num(), *TriggerTypeToString(NewType));
}

void UDataEditorLibrary::BatchSetEffectDuration(const TArray<UEffectDataAsset*>& Targets, float NewDuration)
{
	const float Clamped = FMath::Max(0.01f, NewDuration);
	if (!FMath::IsNearlyEqual(Clamped, NewDuration))
	{
		UE_LOG(LogDataEditor, Warning, TEXT("BatchSetEffectDuration: input %.2f clamped to %.2f (must be > 0)"), NewDuration, Clamped);
	}
	const FScopedTransaction Tx(LOCTEXT("BatchSetEffectDuration", "Batch Set Effect Duration"));
	for (UEffectDataAsset* DA : Targets)
	{
		if (!DA) continue;
		DA->Modify();
		DA->Duration = Clamped;
		DA->MarkPackageDirty();
	}
	UE_LOG(LogDataEditor, Log, TEXT("BatchSetEffectDuration: %d assets set to %.2f"), Targets.Num(), Clamped);
}

void UDataEditorLibrary::BatchSetEffectMaxStack(const TArray<UEffectDataAsset*>& Targets, int32 NewMaxStack)
{
	const int32 Clamped = FMath::Max(1, NewMaxStack);
	if (Clamped != NewMaxStack)
	{
		UE_LOG(LogDataEditor, Warning, TEXT("BatchSetEffectMaxStack: input %d clamped to %d (must be >= 1)"), NewMaxStack, Clamped);
	}
	const FScopedTransaction Tx(LOCTEXT("BatchSetEffectMaxStack", "Batch Set Effect MaxStack"));
	for (UEffectDataAsset* DA : Targets)
	{
		if (!DA) continue;
		DA->Modify();
		DA->MaxStack = Clamped;
		DA->MarkPackageDirty();
	}
	UE_LOG(LogDataEditor, Log, TEXT("BatchSetEffectMaxStack: %d assets set to %d"), Targets.Num(), Clamped);
}

int32 UDataEditorLibrary::PrepareRuneIdTagIni()
{
	const TArray<URuneDataAsset*> All = GetAllRuneDAs();

	const FString IniPath = FPaths::ProjectConfigDir() / TEXT("Tags") / TEXT("RuneIDs.ini");

	// 健壮性：目录不存在时创建
	IPlatformFile& PF = FPlatformFileManager::Get().GetPlatformFile();
	const FString IniDir = FPaths::GetPath(IniPath);
	if (!PF.DirectoryExists(*IniDir) && !PF.CreateDirectoryTree(*IniDir))
	{
		UE_LOG(LogDataEditor, Error, TEXT("PrepareRuneIdTagIni: failed to create directory %s"), *IniDir);
		return 0;
	}

	FString Existing;
	const bool bExisted = FFileHelper::LoadFileToString(Existing, *IniPath);

	// 文件不存在或缺 section header 时补全（GameplayTagsManager 必须有 section header 才能识别）
	const TCHAR* SectionHeader = TEXT("[/Script/GameplayTags.GameplayTagsList]");
	if (!bExisted)
	{
		Existing.Empty();
	}
	bool bSectionHeaderAdded = false;
	if (!Existing.Contains(SectionHeader))
	{
		Existing = FString(SectionHeader) + TEXT("\n") + Existing;
		bSectionHeaderAdded = true;
	}

	// 按 Tag 字符串去重（同一 RuneID 多 DA 共享时只写一条；DevComment 选第一个 DA 名）
	TMap<FString, FString> NewTagToLine; // TagString -> Full ini line
	for (URuneDataAsset* DA : All)
	{
		if (!DA) continue;
		const FRuneConfig& Cfg = DA->RuneInfo.RuneConfig;
		if (Cfg.RuneIdTag.IsValid()) continue;
		if (Cfg.RuneID <= 0) continue;

		const FString TagString = FString::Printf(TEXT("Rune.ID.Legacy_%d"), Cfg.RuneID);

		// 文件里已含此 Tag 则跳过
		const FString TagMarker = FString::Printf(TEXT("Tag=\"%s\""), *TagString);
		if (Existing.Contains(TagMarker))
		{
			continue;
		}

		// 同一 RuneID 在多个 DA 上时，仅保留第一次遇到的 DevComment
		if (!NewTagToLine.Contains(TagString))
		{
			const FString Line = FString::Printf(
				TEXT("GameplayTagList=(Tag=\"%s\",DevComment=\"Auto-migrated from legacy RuneID=%d (first seen on %s)\")"),
				*TagString, Cfg.RuneID, *DA->GetName());
			NewTagToLine.Add(TagString, Line);
		}
	}

	if (NewTagToLine.Num() == 0)
	{
		// 没有新增 Tag 但 .ini 缺 header 时仍需落盘修复
		if (bSectionHeaderAdded || !bExisted)
		{
			if (!FFileHelper::SaveStringToFile(Existing, *IniPath, FFileHelper::EEncodingOptions::ForceUTF8))
			{
				UE_LOG(LogDataEditor, Error, TEXT("PrepareRuneIdTagIni: failed to write %s (read-only? source-controlled?)"), *IniPath);
				return 0;
			}
			UE_LOG(LogDataEditor, Log, TEXT("PrepareRuneIdTagIni: wrote section header to %s (no tags appended)"), *IniPath);
		}
		else
		{
			UE_LOG(LogDataEditor, Log, TEXT("PrepareRuneIdTagIni: no new tags to add (ini up to date)"));
		}
		return 0;
	}

	FString Append = Existing.EndsWith(TEXT("\n")) ? FString() : TEXT("\n");
	for (const TPair<FString, FString>& Pair : NewTagToLine)
	{
		Append += Pair.Value + TEXT("\n");
	}
	if (!FFileHelper::SaveStringToFile(Existing + Append, *IniPath, FFileHelper::EEncodingOptions::ForceUTF8))
	{
		UE_LOG(LogDataEditor, Error, TEXT("PrepareRuneIdTagIni: failed to write %s (read-only? source-controlled?)"), *IniPath);
		return 0;
	}

	UE_LOG(LogDataEditor, Log, TEXT("PrepareRuneIdTagIni: appended %d new tag lines to %s. RESTART THE EDITOR, then call ApplyRuneIdTagsAfterRestart()."),
		NewTagToLine.Num(), *IniPath);
	return NewTagToLine.Num();
}

int32 UDataEditorLibrary::ApplyRuneIdTagsAfterRestart()
{
	const TArray<URuneDataAsset*> All = GetAllRuneDAs();
	const FScopedTransaction Tx(LOCTEXT("ApplyRuneIdTags", "Apply Migrated RuneIdTag"));

	int32 Applied = 0;
	int32 Skipped = 0;

	for (URuneDataAsset* DA : All)
	{
		if (!DA) continue;
		FRuneConfig& Cfg = DA->RuneInfo.RuneConfig;
		if (Cfg.RuneIdTag.IsValid()) continue;
		if (Cfg.RuneID <= 0) continue;

		const FString TagString = FString::Printf(TEXT("Rune.ID.Legacy_%d"), Cfg.RuneID);
		const FName TagName(*TagString);
		const FGameplayTag NewTag = UGameplayTagsManager::Get().RequestGameplayTag(TagName, /*bErrorIfNotFound=*/ false);

		if (!NewTag.IsValid())
		{
			++Skipped;
			UE_LOG(LogDataEditor, Warning, TEXT("ApplyRuneIdTagsAfterRestart: tag %s not registered yet for %s. Run PrepareRuneIdTagIni and restart editor."),
				*TagString, *DA->GetName());
			continue;
		}

		DA->Modify();
		Cfg.RuneIdTag = NewTag;
		DA->MarkPackageDirty();
		++Applied;
	}

	UE_LOG(LogDataEditor, Log, TEXT("ApplyRuneIdTagsAfterRestart: applied %d, skipped %d (of %d total RuneDA)"),
		Applied, Skipped, All.Num());
	return Applied;
}

int32 UDataEditorLibrary::VerifyAccessorParity()
{
	const TArray<URuneDataAsset*> All = GetAllRuneDAs();
	int32 DiffCount = 0;
	for (URuneDataAsset* DA : All)
	{
		if (!DA) continue;
		const FRuneConfig& F = DA->RuneInfo.RuneConfig;
		bool bDiff = false;

		if (DA->GetGoldCost() != F.GoldCost)            { bDiff = true; UE_LOG(LogDataEditor, Warning, TEXT("[Parity] %s: GoldCost diff"), *DA->GetName()); }
		if (DA->GetRarity()   != F.Rarity)              { bDiff = true; UE_LOG(LogDataEditor, Warning, TEXT("[Parity] %s: Rarity diff"), *DA->GetName()); }
		if (DA->GetTriggerType() != F.TriggerType)      { bDiff = true; UE_LOG(LogDataEditor, Warning, TEXT("[Parity] %s: TriggerType diff"), *DA->GetName()); }
		if (DA->GetRuneName() != F.RuneName)            { bDiff = true; UE_LOG(LogDataEditor, Warning, TEXT("[Parity] %s: RuneName diff"), *DA->GetName()); }
		if (DA->GetRuneIdTag() != F.RuneIdTag)          { bDiff = true; UE_LOG(LogDataEditor, Warning, TEXT("[Parity] %s: RuneIdTag diff"), *DA->GetName()); }

		if (bDiff)
		{
			++DiffCount;
		}
	}
	UE_LOG(LogDataEditor, Log, TEXT("VerifyAccessorParity: %d / %d RuneDA have field/accessor diff"), DiffCount, All.Num());
	return DiffCount;
}

FString UDataEditorLibrary::ExportRuneDAsToCSV(const FString& OutFilePath)
{
	const TArray<URuneDataAsset*> All = GetAllRuneDAs();

	FString Path = OutFilePath;
	if (Path.IsEmpty())
	{
		const FString Stamp = FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S"));
		Path = FPaths::ProjectSavedDir() / TEXT("Balance") / FString::Printf(TEXT("RuneNumbers_%s.csv"), *Stamp);
	}
	IPlatformFile& PF = FPlatformFileManager::Get().GetPlatformFile();
	PF.CreateDirectoryTree(*FPaths::GetPath(Path));

	FString Csv;
	Csv += TEXT("AssetName,RuneIdTag,LegacyRuneID,RuneName,RuneType,Rarity,TriggerType,GoldCost,SellPrice,ChainRole,FlowAsset,TuningScalarCount\n");
	for (URuneDataAsset* DA : All)
	{
		if (!DA) continue;
		Csv += FString::Printf(TEXT("%s,%s,%d,%s,%d,%s,%s,%d,%d,%d,%s,%d\n"),
			*CsvEscape(DA->GetName()),
			*CsvEscape(DA->GetRuneIdTag().ToString()),
			DA->GetLegacyRuneID(),
			*CsvEscape(DA->GetRuneName().ToString()),
			static_cast<int32>(DA->GetRuneType()),
			*RarityToString(DA->GetRarity()),
			*TriggerTypeToString(DA->GetTriggerType()),
			DA->GetGoldCost(),
			DA->GetSellPrice(),
			static_cast<int32>(DA->GetChainRole()),
			*CsvEscape(GetNameSafe(DA->GetFlowAsset())),
			DA->GetTuningScalars().Num()
		);
	}

	if (!FFileHelper::SaveStringToFile(Csv, *Path, FFileHelper::EEncodingOptions::ForceUTF8))
	{
		UE_LOG(LogDataEditor, Warning, TEXT("ExportRuneDAsToCSV: failed to write %s"), *Path);
		return FString();
	}
	UE_LOG(LogDataEditor, Log, TEXT("ExportRuneDAsToCSV: wrote %d rows to %s"), All.Num(), *Path);
	return Path;
}

FString UDataEditorLibrary::ExportEffectDAsToCSV(const FString& OutFilePath)
{
	const TArray<UEffectDataAsset*> All = GetAllEffectDAs();

	FString Path = OutFilePath;
	if (Path.IsEmpty())
	{
		const FString Stamp = FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S"));
		Path = FPaths::ProjectSavedDir() / TEXT("Balance") / FString::Printf(TEXT("EffectNumbers_%s.csv"), *Stamp);
	}
	IPlatformFile& PF = FPlatformFileManager::Get().GetPlatformFile();
	PF.CreateDirectoryTree(*FPaths::GetPath(Path));

	FString Csv;
	Csv += TEXT("AssetName,EffectIdTag,DurationType,Duration,Period,UniqueType,StackType,MaxStack,FragmentCount\n");
	for (UEffectDataAsset* DA : All)
	{
		if (!DA) continue;
		Csv += FString::Printf(TEXT("%s,%s,%s,%.2f,%.2f,%d,%d,%d,%d\n"),
			*CsvEscape(DA->GetName()),
			*CsvEscape(DA->GetEffectIdTag().ToString()),
			*DurationTypeToString(DA->GetDurationType()),
			DA->GetDuration(),
			DA->GetPeriod(),
			static_cast<int32>(DA->GetUniqueType()),
			static_cast<int32>(DA->GetStackType()),
			DA->GetMaxStack(),
			DA->GetFragments().Num()
		);
	}

	if (!FFileHelper::SaveStringToFile(Csv, *Path, FFileHelper::EEncodingOptions::ForceUTF8))
	{
		UE_LOG(LogDataEditor, Warning, TEXT("ExportEffectDAsToCSV: failed to write %s"), *Path);
		return FString();
	}
	UE_LOG(LogDataEditor, Log, TEXT("ExportEffectDAsToCSV: wrote %d rows to %s"), All.Num(), *Path);
	return Path;
}

#undef LOCTEXT_NAMESPACE
