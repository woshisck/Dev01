#include "UI/ActiveSkillSetupCommandlet.h"

#include "AbilitySystem/Abilities/GA_ActiveSkill_ShieldBurst.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Commandlets/CommandletReportUtils.h"
#include "Data/ActiveSkillDataAsset.h"
#include "Engine/DataTable.h"
#include "FileHelpers.h"
#include "GameplayTagsManager.h"
#include "MetaProgression/MetaTypes.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"

namespace
{
	const FString ActiveSkillAssetPath = TEXT("/Game/Code/Core/ActiveSkills/DA_ActiveSkill_ShieldBurst");
	const FString MetaUpgradeNodeTablePath = TEXT("/Game/MetaProgression/DT_MetaUpgradeNodes");
	const FString ActiveSkillSetupReportFileName = TEXT("ActiveSkillSetupReport.md");

	FString ToObjectPath(const FString& PackagePath)
	{
		const FString AssetName = FPackageName::GetLongPackageAssetName(PackagePath);
		return PackagePath + TEXT(".") + AssetName;
	}
}

UActiveSkillSetupCommandlet::UActiveSkillSetupCommandlet()
{
	IsClient = false;
	IsEditor = true;
	LogToConsole = true;
}

int32 UActiveSkillSetupCommandlet::Main(const FString& Params)
{
	const bool bApply = Params.Contains(TEXT("Apply"), ESearchCase::IgnoreCase);
	const bool bDryRun = !bApply;

	TArray<FString> ReportLines;
	TArray<UPackage*> DirtyPackages;
	ReportLines.Add(TEXT("# Active Skill Setup Report"));
	ReportLines.Add(FString::Printf(TEXT("- Mode: %s"), bDryRun ? TEXT("DryRun") : TEXT("Apply")));
	ReportLines.Add(TEXT(""));

	UActiveSkillDataAsset* SkillAsset = Cast<UActiveSkillDataAsset>(
		StaticLoadObject(UActiveSkillDataAsset::StaticClass(), nullptr, *ToObjectPath(ActiveSkillAssetPath)));

	if (!SkillAsset)
	{
		ReportLines.Add(FString::Printf(TEXT("- %s `%s`."), bDryRun ? TEXT("Would create") : TEXT("Created"), *ActiveSkillAssetPath));
		if (!bDryRun)
		{
			UPackage* Package = CreatePackage(*ActiveSkillAssetPath);
			const FString AssetName = FPackageName::GetLongPackageAssetName(ActiveSkillAssetPath);
			SkillAsset = NewObject<UActiveSkillDataAsset>(
				Package,
				UActiveSkillDataAsset::StaticClass(),
				*AssetName,
				RF_Public | RF_Standalone | RF_Transactional);
			if (!SkillAsset)
			{
				ReportLines.Add(TEXT("- Failed to create active skill asset."));
				FString ReportPath;
				FString SharedReportPath;
				DevKitEditorCommandletReports::SaveReportLines(ActiveSkillSetupReportFileName, ReportLines, ReportPath, SharedReportPath);
				return 1;
			}

			FAssetRegistryModule::AssetCreated(SkillAsset);
			DirtyPackages.AddUnique(Package);
		}
	}
	else
	{
		ReportLines.Add(FString::Printf(TEXT("- Found `%s`."), *ActiveSkillAssetPath));
	}

	if (SkillAsset && !bDryRun)
	{
		SkillAsset->Modify();
		SkillAsset->Config.SkillId = TEXT("ActiveSkill.ShieldBurst");
		SkillAsset->Config.DisplayName = FText::FromString(TEXT("Shield Burst"));
		SkillAsset->Config.AbilityClass = UGA_ActiveSkill_ShieldBurst::StaticClass();
		SkillAsset->Config.Cooldown = 120.0f;
		SkillAsset->MarkPackageDirty();
		DirtyPackages.AddUnique(SkillAsset->GetPackage());
	}

	ReportLines.Add(TEXT("- `DA_ActiveSkill_ShieldBurst` uses `UGA_ActiveSkill_ShieldBurst`, cooldown 120s, buff duration 60s."));

	UDataTable* NodeTable = Cast<UDataTable>(
		StaticLoadObject(UDataTable::StaticClass(), nullptr, *ToObjectPath(MetaUpgradeNodeTablePath)));
	if (!NodeTable)
	{
		ReportLines.Add(FString::Printf(TEXT("- Missing `%s`; skill unlock nodes were not updated."), *MetaUpgradeNodeTablePath));
	}
	else if (!bDryRun)
	{
		NodeTable->Modify();

		FMetaUpgradeNodeRow UnlockRow;
		UnlockRow.DisplayName = FText::FromString(TEXT("Active Skill Unlock"));
		UnlockRow.Side = EMetaSide::Mystic;
		UnlockRow.MaxLevel = 1;
		UnlockRow.EffectType = EMetaUpgradeEffectType::FeatureUnlock;
		UnlockRow.FeatureTag = FGameplayTag::RequestGameplayTag(TEXT("Feature.Combat.ActiveSkill"), false);
		FMetaCurrencyCost UnlockCost;
		UnlockCost.CurrencyTag = FGameplayTag::RequestGameplayTag(TEXT("Currency.Meta.MysticPoint"), false);
		UnlockCost.Amount = 30;
		UnlockRow.CostsPerLevel.Add(UnlockCost);
#if WITH_EDITORONLY_DATA
		UnlockRow.EditorPositionX = 640.0f;
		UnlockRow.EditorPositionY = 0.0f;
#endif
		NodeTable->AddRow(TEXT("Node.Skill.Unlock"), UnlockRow);

		FMetaUpgradeNodeRow Slot2Row;
		Slot2Row.DisplayName = FText::FromString(TEXT("Active Skill Slot 2"));
		Slot2Row.Side = EMetaSide::Mystic;
		Slot2Row.MaxLevel = 1;
		Slot2Row.Prerequisites.Add(TEXT("Node.Skill.Unlock"));
		Slot2Row.EffectType = EMetaUpgradeEffectType::FeatureUnlock;
		Slot2Row.FeatureTag = FGameplayTag::RequestGameplayTag(TEXT("Feature.Combat.ActiveSkill.Slot2"), false);
		FMetaCurrencyCost Slot2Cost;
		Slot2Cost.CurrencyTag = FGameplayTag::RequestGameplayTag(TEXT("Currency.Meta.MysticPoint"), false);
		Slot2Cost.Amount = 50;
		Slot2Row.CostsPerLevel.Add(Slot2Cost);
#if WITH_EDITORONLY_DATA
		Slot2Row.EditorPositionX = 960.0f;
		Slot2Row.EditorPositionY = 0.0f;
#endif
		NodeTable->AddRow(TEXT("Node.Skill.Slot2"), Slot2Row);

		NodeTable->MarkPackageDirty();
		DirtyPackages.AddUnique(NodeTable->GetPackage());
		ReportLines.Add(TEXT("- Updated `DT_MetaUpgradeNodes`: `Node.Skill.Unlock` costs 30 MysticPoint and unlocks `Feature.Combat.ActiveSkill`; `Node.Skill.Slot2` costs 50 MysticPoint, depends on unlock, and unlocks `Feature.Combat.ActiveSkill.Slot2`."));
	}
	else
	{
		ReportLines.Add(TEXT("- Would update `DT_MetaUpgradeNodes` skill unlock rows."));
	}

	if (!bDryRun && DirtyPackages.Num() > 0)
	{
		UEditorLoadingAndSavingUtils::SavePackages(DirtyPackages, false);
	}

	FString ReportPath;
	FString SharedReportPath;
	DevKitEditorCommandletReports::SaveReportLines(ActiveSkillSetupReportFileName, ReportLines, ReportPath, SharedReportPath);
	UE_LOG(LogTemp, Display, TEXT("Active skill setup finished. Report: %s Shared: %s"), *ReportPath, *SharedReportPath);
	return 0;
}
