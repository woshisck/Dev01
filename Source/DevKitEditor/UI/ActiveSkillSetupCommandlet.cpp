#include "UI/ActiveSkillSetupCommandlet.h"

#include "AbilitySystem/Abilities/GA_ActiveSkill_ShieldBurst.h"
#include "AssetToolsModule.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Widget.h"
#include "Engine/Blueprint.h"
#include "Commandlets/CommandletReportUtils.h"
#include "Data/ActiveSkillDataAsset.h"
#include "Engine/DataTable.h"
#include "Factories/BlueprintFactory.h"
#include "FileHelpers.h"
#include "GameplayTagsManager.h"
#include "IAssetTools.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "MetaProgression/MetaTypes.h"
#include "Misc/PackageName.h"
#include "UI/ActiveSkillBarWidget.h"
#include "UI/ActiveSkillLoadoutWidget.h"
#include "UI/YogUIRegistry.h"
#include "UObject/Package.h"
#include "WidgetBlueprint.h"
#include "WidgetBlueprintFactory.h"
#include "World/HubFacilityActor.h"

namespace
{
	const FString ActiveSkillBarWidgetPath = TEXT("/Game/UI/ActiveSkill/WBP_ActiveSkillBar");
	const FString ActiveSkillLoadoutWidgetPath = TEXT("/Game/UI/ActiveSkill/WBP_ActiveSkillLoadout");
	const FString UIRegistryPath = TEXT("/Game/UI/DA_YogUIRegistry");
	const FString ActiveSkillAssetPath = TEXT("/Game/Code/Core/ActiveSkills/DA_ActiveSkill_ShieldBurst");
	const FString ActiveSkillTerminalBlueprintPath = TEXT("/Game/Code/Core/Hub/BP_HubActiveSkillTerminal");
	const FString MetaUpgradeNodeTablePath = TEXT("/Game/MetaProgression/DT_MetaUpgradeNodes");
	const FString ActiveSkillSetupReportFileName = TEXT("ActiveSkillSetupReport.md");

	FString ToObjectPath(const FString& PackagePath)
	{
		const FString AssetName = FPackageName::GetLongPackageAssetName(PackagePath);
		return PackagePath + TEXT(".") + AssetName;
	}

	template<typename AssetT>
	AssetT* LoadAssetByPackagePath(const FString& PackagePath)
	{
		return Cast<AssetT>(StaticLoadObject(AssetT::StaticClass(), nullptr, *ToObjectPath(PackagePath)));
	}

	UBlueprint* CreateBlueprintAsset(const FString& PackagePath, UClass* ParentClass, bool bDryRun, TArray<FString>& ReportLines, TArray<UPackage*>& DirtyPackages)
	{
		if (UBlueprint* Existing = LoadAssetByPackagePath<UBlueprint>(PackagePath))
		{
			ReportLines.Add(FString::Printf(TEXT("- Found `%s`."), *PackagePath));
			return Existing;
		}

		ReportLines.Add(FString::Printf(TEXT("- %s Blueprint `%s`."), bDryRun ? TEXT("Would create") : TEXT("Created"), *PackagePath));
		if (bDryRun)
		{
			return nullptr;
		}

		IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
		UBlueprintFactory* Factory = NewObject<UBlueprintFactory>();
		Factory->BlueprintType = BPTYPE_Normal;
		Factory->ParentClass = ParentClass;

		UBlueprint* Blueprint = Cast<UBlueprint>(AssetTools.CreateAsset(
			FPackageName::GetLongPackageAssetName(PackagePath),
			FPackageName::GetLongPackagePath(PackagePath),
			UBlueprint::StaticClass(),
			Factory));
		if (Blueprint)
		{
			FKismetEditorUtilities::CompileBlueprint(Blueprint);
			FAssetRegistryModule::AssetCreated(Blueprint);
			Blueprint->MarkPackageDirty();
			DirtyPackages.AddUnique(Blueprint->GetPackage());
		}
		return Blueprint;
	}

	UWidgetBlueprint* CreateWidgetBlueprintAsset(const FString& PackagePath, UClass* ParentClass, bool bDryRun, TArray<FString>& ReportLines, TArray<UPackage*>& DirtyPackages)
	{
		if (UWidgetBlueprint* Existing = LoadAssetByPackagePath<UWidgetBlueprint>(PackagePath))
		{
			ReportLines.Add(FString::Printf(TEXT("- Found `%s`."), *PackagePath));
			if (!bDryRun && Existing->ParentClass != ParentClass)
			{
				Existing->Modify();
				Existing->ParentClass = ParentClass;
				FBlueprintEditorUtils::RefreshAllNodes(Existing);
				FBlueprintEditorUtils::MarkBlueprintAsModified(Existing);
				FKismetEditorUtilities::CompileBlueprint(Existing);
				Existing->MarkPackageDirty();
				DirtyPackages.AddUnique(Existing->GetPackage());
				ReportLines.Add(FString::Printf(TEXT("- Reparented `%s` to `%s`."), *PackagePath, *GetNameSafe(ParentClass)));
			}
			return Existing;
		}

		ReportLines.Add(FString::Printf(
			TEXT("- %s Widget Blueprint `%s` with parent `%s`."),
			bDryRun ? TEXT("Would create") : TEXT("Created"),
			*PackagePath,
			*GetNameSafe(ParentClass)));
		if (bDryRun)
		{
			return nullptr;
		}

		IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
		UWidgetBlueprintFactory* Factory = NewObject<UWidgetBlueprintFactory>();
		Factory->BlueprintType = BPTYPE_Normal;
		Factory->ParentClass = ParentClass;

		UWidgetBlueprint* WidgetBlueprint = Cast<UWidgetBlueprint>(AssetTools.CreateAsset(
			FPackageName::GetLongPackageAssetName(PackagePath),
			FPackageName::GetLongPackagePath(PackagePath),
			UWidgetBlueprint::StaticClass(),
			Factory));
		if (WidgetBlueprint)
		{
			FKismetEditorUtilities::CompileBlueprint(WidgetBlueprint);
			FAssetRegistryModule::AssetCreated(WidgetBlueprint);
			WidgetBlueprint->MarkPackageDirty();
			DirtyPackages.AddUnique(WidgetBlueprint->GetPackage());
		}
		return WidgetBlueprint;
	}

	bool EnsureRuntimeOnlyWidgetBlueprint(UWidgetBlueprint* WidgetBlueprint, const FString& PackagePath, TArray<FString>& ReportLines, TArray<UPackage*>& DirtyPackages)
	{
		if (!WidgetBlueprint || !WidgetBlueprint->WidgetTree)
		{
			return false;
		}

		TArray<UWidget*> ExistingWidgets;
		TArray<UObject*> ExistingObjects;
		GetObjectsWithOuter(WidgetBlueprint->WidgetTree, ExistingObjects, true);
		for (UObject* ExistingObject : ExistingObjects)
		{
			if (UWidget* ExistingWidget = Cast<UWidget>(ExistingObject))
			{
				ExistingWidgets.AddUnique(ExistingWidget);
			}
		}

		if (!WidgetBlueprint->WidgetTree->RootWidget && ExistingWidgets.Num() == 0)
		{
			return false;
		}

		WidgetBlueprint->Modify();
		WidgetBlueprint->WidgetTree->Modify();
		if (WidgetBlueprint->WidgetTree->RootWidget)
		{
			WidgetBlueprint->WidgetTree->RemoveWidget(WidgetBlueprint->WidgetTree->RootWidget);
			WidgetBlueprint->WidgetTree->RootWidget = nullptr;
		}

		for (UWidget* ExistingWidget : ExistingWidgets)
		{
			if (!ExistingWidget)
			{
				continue;
			}

			WidgetBlueprint->WidgetTree->RemoveWidget(ExistingWidget);
			const FString OldName = FString::Printf(
				TEXT("RuntimeOnlyOld_%s_%s"),
				*ExistingWidget->GetName(),
				*FGuid::NewGuid().ToString(EGuidFormats::Digits));
			ExistingWidget->Rename(*OldName, GetTransientPackage(), REN_DontCreateRedirectors | REN_NonTransactional);
		}

		FKismetEditorUtilities::CompileBlueprint(WidgetBlueprint);
		WidgetBlueprint->MarkPackageDirty();
		DirtyPackages.AddUnique(WidgetBlueprint->GetPackage());
		ReportLines.Add(FString::Printf(TEXT("- Cleared designer widgets in `%s`; runtime C++ builds the layout."), *PackagePath));
		return true;
	}

	void ConfigureUIRegistry(bool bDryRun, TArray<FString>& ReportLines, TArray<UPackage*>& DirtyPackages)
	{
		ReportLines.Add(FString::Printf(
			TEXT("- %s `%s` ActiveSkillBar entry -> `WBP_ActiveSkillBar`, layer Game, ZOrder 0."),
			bDryRun ? TEXT("Would configure") : TEXT("Configured"),
			*UIRegistryPath));
		if (bDryRun)
		{
			return;
		}

		UYogUIRegistry* Registry = LoadAssetByPackagePath<UYogUIRegistry>(UIRegistryPath);
		if (!Registry)
		{
			ReportLines.Add(FString::Printf(TEXT("- Missing `%s`; UI registry was not updated."), *UIRegistryPath));
			return;
		}

		const FSoftObjectPath ActiveSkillBarClassPath(TEXT("/Game/UI/ActiveSkill/WBP_ActiveSkillBar.WBP_ActiveSkillBar_C"));
		FYogUIRegistryEntry* ActiveSkillEntry = Registry->Entries.FindByPredicate([](const FYogUIRegistryEntry& Entry)
		{
			return Entry.ScreenId == EYogUIScreenId::ActiveSkillBar;
		});

		if (!ActiveSkillEntry)
		{
			Registry->Modify();
			FYogUIRegistryEntry NewEntry;
			NewEntry.ScreenId = EYogUIScreenId::ActiveSkillBar;
			ActiveSkillEntry = &Registry->Entries.Add_GetRef(NewEntry);
		}

		if (ActiveSkillEntry->WidgetClass.ToSoftObjectPath() != ActiveSkillBarClassPath
			|| ActiveSkillEntry->Layer != EYogUILayer::Game
			|| ActiveSkillEntry->ZOrder != 0)
		{
			Registry->Modify();
			ActiveSkillEntry->WidgetClass = TSoftClassPtr<UUserWidget>(ActiveSkillBarClassPath);
			ActiveSkillEntry->Layer = EYogUILayer::Game;
			ActiveSkillEntry->ZOrder = 0;
			Registry->MarkPackageDirty();
			DirtyPackages.AddUnique(Registry->GetPackage());
		}
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

	UWidgetBlueprint* ActiveSkillBarWidget = CreateWidgetBlueprintAsset(
		ActiveSkillBarWidgetPath,
		UActiveSkillBarWidget::StaticClass(),
		bDryRun,
		ReportLines,
		DirtyPackages);
	if (ActiveSkillBarWidget && !bDryRun)
	{
		EnsureRuntimeOnlyWidgetBlueprint(ActiveSkillBarWidget, ActiveSkillBarWidgetPath, ReportLines, DirtyPackages);
		FKismetEditorUtilities::CompileBlueprint(ActiveSkillBarWidget);
	}

	UWidgetBlueprint* ActiveSkillLoadoutWidget = CreateWidgetBlueprintAsset(
		ActiveSkillLoadoutWidgetPath,
		UActiveSkillLoadoutWidget::StaticClass(),
		bDryRun,
		ReportLines,
		DirtyPackages);
	if (ActiveSkillLoadoutWidget && !bDryRun)
	{
		EnsureRuntimeOnlyWidgetBlueprint(ActiveSkillLoadoutWidget, ActiveSkillLoadoutWidgetPath, ReportLines, DirtyPackages);
		FKismetEditorUtilities::CompileBlueprint(ActiveSkillLoadoutWidget);
	}

	ConfigureUIRegistry(bDryRun, ReportLines, DirtyPackages);

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

	UBlueprint* TerminalBlueprint = CreateBlueprintAsset(
		ActiveSkillTerminalBlueprintPath,
		AHubFacilityActor::StaticClass(),
		bDryRun,
		ReportLines,
		DirtyPackages);
	if (TerminalBlueprint && !bDryRun)
	{
		FKismetEditorUtilities::CompileBlueprint(TerminalBlueprint);
		if (AHubFacilityActor* CDO = Cast<AHubFacilityActor>(TerminalBlueprint->GeneratedClass->GetDefaultObject()))
		{
			CDO->Modify();
			UClass* LoadoutWidgetClass = UActiveSkillLoadoutWidget::StaticClass();
			if (ActiveSkillLoadoutWidget && ActiveSkillLoadoutWidget->GeneratedClass
				&& ActiveSkillLoadoutWidget->GeneratedClass->IsChildOf(UActiveSkillLoadoutWidget::StaticClass()))
			{
				LoadoutWidgetClass = ActiveSkillLoadoutWidget->GeneratedClass;
			}

			CDO->FacilityDisplayName = FText::FromString(TEXT("Active Skill Terminal"));
			CDO->WidgetClass = LoadoutWidgetClass;
			CDO->RequiredFeatureTag = FGameplayTag::RequestGameplayTag(TEXT("Feature.Combat.ActiveSkill"), false);
			FBlueprintEditorUtils::MarkBlueprintAsModified(TerminalBlueprint);
			FKismetEditorUtilities::CompileBlueprint(TerminalBlueprint);
			TerminalBlueprint->MarkPackageDirty();
			DirtyPackages.AddUnique(TerminalBlueprint->GetPackage());
			ReportLines.Add(TEXT("- Configured `BP_HubActiveSkillTerminal`: widget `WBP_ActiveSkillLoadout`, required feature `Feature.Combat.ActiveSkill`."));
		}
	}
	ReportLines.Add(TEXT("- Hub terminal is spawned at runtime by `AYogGameMode::EnsureHubActiveSkillTerminal` when the active room is a HubRoom."));

	UDataTable* NodeTable = Cast<UDataTable>(
		StaticLoadObject(UDataTable::StaticClass(), nullptr, *ToObjectPath(MetaUpgradeNodeTablePath)));
	if (!NodeTable)
	{
		ReportLines.Add(FString::Printf(TEXT("- Missing `%s`; skill unlock nodes were not updated."), *MetaUpgradeNodeTablePath));
	}
	else if (!bDryRun)
	{
		bool bTableDirty = false;

		if (!NodeTable->FindRow<FMetaUpgradeNodeRow>(TEXT("Node.Skill.Unlock"), TEXT(""), false))
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
			bTableDirty = true;
			ReportLines.Add(TEXT("- Added `Node.Skill.Unlock` (30 MysticPoint, unlocks `Feature.Combat.ActiveSkill`)."));
		}
		else
		{
			ReportLines.Add(TEXT("- `Node.Skill.Unlock` already exists; skipped."));
		}

		if (!NodeTable->FindRow<FMetaUpgradeNodeRow>(TEXT("Node.Skill.Slot2"), TEXT(""), false))
		{
			NodeTable->Modify();
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
			bTableDirty = true;
			ReportLines.Add(TEXT("- Added `Node.Skill.Slot2` (50 MysticPoint, requires Node.Skill.Unlock, unlocks `Feature.Combat.ActiveSkill.Slot2`)."));
		}
		else
		{
			ReportLines.Add(TEXT("- `Node.Skill.Slot2` already exists; skipped."));
		}

		if (bTableDirty)
		{
			NodeTable->MarkPackageDirty();
			DirtyPackages.AddUnique(NodeTable->GetPackage());
		}
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
