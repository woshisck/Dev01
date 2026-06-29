#include "MaterialBatch/MaterialBatchAuditCommandlet.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Commandlets/CommandletReportUtils.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Level.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "FileHelpers.h"
#include "GameFramework/Actor.h"
#include "MaterialBatch/MaterialBatchAuditHelpers.h"
#include "MaterialBatch/MaterialBatchCandidateRules.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "Modules/ModuleManager.h"

namespace
{
const TCHAR* MaterialBatchAuditReportFileName = TEXT("MaterialBatchAuditReport.md");

struct FMaterialBatchAuditRow
{
	FString SourceType;
	FString PackageName;
	FString AssetName;
	FString ActorName;
	FString ComponentName;
	FString Tags;
	int32 MaterialSlotCount = 0;
	int32 LodCount = 0;
	bool bEligible = false;
	EMaterialBatchCandidateRejectReason Reason = EMaterialBatchCandidateRejectReason::None;
};

FString GetParamValue(const FString& Params, const TCHAR* Key, const FString& DefaultValue)
{
	FString Value;
	if (FParse::Value(*Params, Key, Value))
	{
		return Value;
	}
	return DefaultValue;
}

int32 GetParamInt(const FString& Params, const TCHAR* Key, int32 DefaultValue)
{
	int32 Value = DefaultValue;
	FParse::Value(*Params, Key, Value);
	return Value;
}

bool HasSwitch(const FString& Params, const TCHAR* SwitchName)
{
	return FParse::Param(*Params, SwitchName);
}

FString JoinTags(const TArray<FName>& Tags)
{
	TArray<FString> Parts;
	Parts.Reserve(Tags.Num());
	for (const FName& Tag : Tags)
	{
		Parts.Add(Tag.ToString());
	}
	return FString::Join(Parts, TEXT(", "));
}

void AddUsage(TArray<FString>& Lines)
{
	Lines.Add(TEXT("## Usage"));
	Lines.Add(TEXT(""));
	Lines.Add(TEXT("```text"));
	Lines.Add(TEXT("UnrealEditor-Cmd.exe DevKit.uproject -run=MaterialBatchAudit -Root=/Game/Art -MaxAssets=500"));
	Lines.Add(TEXT("UnrealEditor-Cmd.exe DevKit.uproject -run=MaterialBatchAudit -Map=/Game/Maps/TestMap -MaxActors=200"));
	Lines.Add(TEXT("```"));
	Lines.Add(TEXT(""));
	Lines.Add(TEXT("- `-Root=/Game/Art`: package root to scan recursively."));
	Lines.Add(TEXT("- `-MaxAssets=500`: load and inspect at most this many StaticMesh assets. Use 0 for no limit."));
	Lines.Add(TEXT("- `-Map=/Game/Maps/TestMap`: load a map and scan placed StaticMeshComponent instances."));
	Lines.Add(TEXT("- `-MaxActors=200`: inspect at most this many actors when `-Map` is provided. Use 0 for no limit."));
	Lines.Add(TEXT("- `-IncludeEngine`: allow non-/Game roots for engine/plugin experiments."));
	Lines.Add(TEXT(""));
}
}

UMaterialBatchAuditCommandlet::UMaterialBatchAuditCommandlet()
{
	IsClient = false;
	IsEditor = true;
	IsServer = false;
	LogToConsole = true;
}

int32 UMaterialBatchAuditCommandlet::Main(const FString& Params)
{
	const FString RootPath = GetParamValue(Params, TEXT("Root="), TEXT("/Game/Art"));
	const FString MapPath = GetParamValue(Params, TEXT("Map="), TEXT(""));
	const int32 MaxAssets = FMath::Max(0, GetParamInt(Params, TEXT("MaxAssets="), 500));
	const int32 MaxActors = FMath::Max(0, GetParamInt(Params, TEXT("MaxActors="), 200));
	const bool bIncludeEngine = HasSwitch(Params, TEXT("IncludeEngine"));
	const bool bScanMap = !MapPath.IsEmpty();

	TArray<FString> ReportLines;
	ReportLines.Add(TEXT("# Material Batch Audit Report"));
	ReportLines.Add(TEXT(""));
	ReportLines.Add(FString::Printf(TEXT("- Root: `%s`"), *RootPath));
	ReportLines.Add(FString::Printf(TEXT("- Map: `%s`"), bScanMap ? *MapPath : TEXT("(not set)")));
	ReportLines.Add(FString::Printf(TEXT("- MaxAssets: %d"), MaxAssets));
	ReportLines.Add(FString::Printf(TEXT("- MaxActors: %d"), MaxActors));
	ReportLines.Add(FString::Printf(TEXT("- IncludeEngine: %s"), bIncludeEngine ? TEXT("true") : TEXT("false")));
	ReportLines.Add(TEXT("- Mode: report-only; no assets are modified or generated."));
	ReportLines.Add(TEXT(""));
	AddUsage(ReportLines);

	if (!bScanMap && !RootPath.StartsWith(TEXT("/Game")) && !bIncludeEngine)
	{
		ReportLines.Add(TEXT("## Result"));
		ReportLines.Add(TEXT(""));
		ReportLines.Add(TEXT("- Failed: root must start with `/Game` unless `-IncludeEngine` is provided."));
		FString SavedReportPath;
		FString SharedReportPath;
		DevKitEditorCommandletReports::SaveReportLines(MaterialBatchAuditReportFileName, ReportLines, SavedReportPath, SharedReportPath);
		UE_LOG(LogTemp, Error, TEXT("MaterialBatchAudit failed; report: %s"), *SharedReportPath);
		return 1;
	}
	if (bScanMap && !MapPath.StartsWith(TEXT("/Game")) && !bIncludeEngine)
	{
		ReportLines.Add(TEXT("## Result"));
		ReportLines.Add(TEXT(""));
		ReportLines.Add(TEXT("- Failed: map must start with `/Game` unless `-IncludeEngine` is provided."));
		FString SavedReportPath;
		FString SharedReportPath;
		DevKitEditorCommandletReports::SaveReportLines(MaterialBatchAuditReportFileName, ReportLines, SavedReportPath, SharedReportPath);
		UE_LOG(LogTemp, Error, TEXT("MaterialBatchAudit failed; report: %s"), *SharedReportPath);
		return 1;
	}

	TArray<FMaterialBatchAuditRow> Rows;
	int32 SourceFoundCount = 0;
	const TCHAR* SourceFoundLabel = bScanMap ? TEXT("StaticMesh components found") : TEXT("StaticMesh assets found");
	const TCHAR* SourceInspectedLabel = bScanMap ? TEXT("StaticMesh components inspected") : TEXT("StaticMesh assets inspected");

	if (bScanMap)
	{
		const FString MapFilename = FMaterialBatchAuditHelpers::ResolveMapFilename(MapPath);
		UWorld* World = UEditorLoadingAndSavingUtils::LoadMap(MapFilename);
		if (!World)
		{
			ReportLines.Add(TEXT("## Result"));
			ReportLines.Add(TEXT(""));
			ReportLines.Add(FString::Printf(TEXT("- Failed: could not load map `%s` from `%s`."), *MapPath, *MapFilename));
			FString SavedReportPath;
			FString SharedReportPath;
			DevKitEditorCommandletReports::SaveReportLines(MaterialBatchAuditReportFileName, ReportLines, SavedReportPath, SharedReportPath);
			UE_LOG(LogTemp, Error, TEXT("MaterialBatchAudit failed; report: %s"), *SharedReportPath);
			return 1;
		}

		int32 ActorsInspected = 0;
		for (ULevel* Level : World->GetLevels())
		{
			if (!Level)
			{
				continue;
			}
			for (AActor* Actor : Level->Actors)
			{
				if (!Actor)
				{
					continue;
				}
				if (MaxActors > 0 && ActorsInspected >= MaxActors)
				{
					break;
				}
				++ActorsInspected;

				TArray<UStaticMeshComponent*> StaticMeshComponents;
				Actor->GetComponents<UStaticMeshComponent>(StaticMeshComponents);
				for (UStaticMeshComponent* Component : StaticMeshComponents)
				{
					++SourceFoundCount;
					UStaticMesh* StaticMesh = Component ? Component->GetStaticMesh() : nullptr;

					FMaterialBatchAuditRow Row;
					Row.SourceType = TEXT("MapComponent");
					Row.PackageName = MapPath;
					Row.AssetName = StaticMesh ? StaticMesh->GetPathName() : TEXT("(no static mesh)");
					Row.ActorName = Actor->GetActorNameOrLabel();
					Row.ComponentName = Component ? Component->GetName() : TEXT("(null)");

					TArray<FName> CombinedTags = Actor->Tags;
					if (Component)
					{
						CombinedTags.Append(Component->ComponentTags);
					}
					Row.Tags = JoinTags(CombinedTags);

					FMaterialBatchComponentScanInput Input;
					Input.bIsStaticMeshComponent = StaticMesh != nullptr;
					Input.bHasStaticMobility = Component && Component->Mobility == EComponentMobility::Static;
					if (StaticMesh)
					{
						Row.MaterialSlotCount = StaticMesh->GetStaticMaterials().Num();
						Row.LodCount = StaticMesh->GetNumLODs();
						Input.MaterialSlotCount = Row.MaterialSlotCount;
						Input.LodCount = Row.LodCount;
					}
					Input = FMaterialBatchCandidateRules::BuildInputFromTags(Input, CombinedTags);

					const FMaterialBatchCandidateDecision Decision = FMaterialBatchCandidateRules::ClassifyComponent(Input);
					Row.bEligible = Decision.bEligible;
					Row.Reason = Decision.Reason;
					Rows.Add(Row);
				}
			}
			if (MaxActors > 0 && ActorsInspected >= MaxActors)
			{
				break;
			}
		}
	}
	else
	{
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
		IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();
		AssetRegistry.SearchAllAssets(true);

		FARFilter Filter;
		Filter.PackagePaths.Add(FName(*RootPath));
		Filter.ClassPaths.Add(UStaticMesh::StaticClass()->GetClassPathName());
		Filter.bRecursivePaths = true;
		Filter.bRecursiveClasses = true;

		TArray<FAssetData> MeshAssets;
		AssetRegistry.GetAssets(Filter, MeshAssets);
		MeshAssets.Sort([](const FAssetData& Left, const FAssetData& Right)
		{
			return Left.PackageName.LexicalLess(Right.PackageName);
		});
		SourceFoundCount = MeshAssets.Num();
		Rows.Reserve(MaxAssets > 0 ? FMath::Min(MaxAssets, MeshAssets.Num()) : MeshAssets.Num());

		int32 InspectedCount = 0;
		for (const FAssetData& AssetData : MeshAssets)
		{
			if (MaxAssets > 0 && InspectedCount >= MaxAssets)
			{
				break;
			}
			++InspectedCount;

			UStaticMesh* StaticMesh = Cast<UStaticMesh>(AssetData.GetAsset());
			FMaterialBatchAuditRow Row;
			Row.SourceType = TEXT("StaticMeshAsset");
			Row.PackageName = AssetData.PackageName.ToString();
			Row.AssetName = AssetData.AssetName.ToString();

			FMaterialBatchComponentScanInput Input;
			Input.bIsStaticMeshComponent = StaticMesh != nullptr;
			Input.bHasStaticMobility = true;
			if (StaticMesh)
			{
				Row.MaterialSlotCount = StaticMesh->GetStaticMaterials().Num();
				Row.LodCount = StaticMesh->GetNumLODs();
				Input.MaterialSlotCount = Row.MaterialSlotCount;
				Input.LodCount = Row.LodCount;
			}

			const FMaterialBatchCandidateDecision Decision = FMaterialBatchCandidateRules::ClassifyComponent(Input);
			Row.bEligible = Decision.bEligible;
			Row.Reason = Decision.Reason;
			Rows.Add(Row);
		}
	}

	int32 EligibleCount = 0;
	TMap<EMaterialBatchCandidateRejectReason, int32> RejectCounts;
	for (const FMaterialBatchAuditRow& Row : Rows)
	{
		if (Row.bEligible)
		{
			++EligibleCount;
		}
		else
		{
			RejectCounts.FindOrAdd(Row.Reason)++;
		}
	}

	ReportLines.Add(TEXT("## Summary"));
	ReportLines.Add(TEXT(""));
	ReportLines.Add(FString::Printf(TEXT("- %s: %d"), SourceFoundLabel, SourceFoundCount));
	ReportLines.Add(FString::Printf(TEXT("- %s: %d"), SourceInspectedLabel, Rows.Num()));
	ReportLines.Add(FString::Printf(TEXT("- Batch candidates: %d"), EligibleCount));
	ReportLines.Add(FString::Printf(TEXT("- Rejected: %d"), Rows.Num() - EligibleCount));
	ReportLines.Add(TEXT(""));

	ReportLines.Add(TEXT("## Reject Reasons"));
	ReportLines.Add(TEXT(""));
	ReportLines.Add(TEXT("| Reason | Count |"));
	ReportLines.Add(TEXT("| --- | ---: |"));
	for (const TPair<EMaterialBatchCandidateRejectReason, int32>& Pair : RejectCounts)
	{
		ReportLines.Add(FString::Printf(TEXT("| %s | %d |"), *FMaterialBatchCandidateRules::RejectReasonToString(Pair.Key), Pair.Value));
	}
	if (RejectCounts.IsEmpty())
	{
		ReportLines.Add(TEXT("| None | 0 |"));
	}
	ReportLines.Add(TEXT(""));

	ReportLines.Add(TEXT("## Candidate Detail"));
	ReportLines.Add(TEXT(""));
	ReportLines.Add(TEXT("| Source | Asset/Map | Actor | Component | Materials | LODs | Tags | Status | Reason |"));
	ReportLines.Add(TEXT("| --- | --- | --- | --- | ---: | ---: | --- | --- | --- |"));
	for (const FMaterialBatchAuditRow& Row : Rows)
	{
		ReportLines.Add(FString::Printf(
			TEXT("| %s | `%s` | `%s` | `%s` | %d | %d | `%s` | %s | %s |"),
			*Row.SourceType,
			*Row.PackageName,
			Row.ActorName.IsEmpty() ? TEXT("") : *Row.ActorName,
			Row.ComponentName.IsEmpty() ? TEXT("") : *Row.ComponentName,
			Row.MaterialSlotCount,
			Row.LodCount,
			Row.Tags.IsEmpty() ? TEXT("") : *Row.Tags,
			Row.bEligible ? TEXT("Candidate") : TEXT("Rejected"),
			*FMaterialBatchCandidateRules::RejectReasonToString(Row.Reason)));
	}

	ReportLines.Add(TEXT(""));
	ReportLines.Add(TEXT("## Next Steps"));
	ReportLines.Add(TEXT(""));
	ReportLines.Add(TEXT("- Review multi-material candidates before enabling merge; one material slot remains the preferred production target."));
	ReportLines.Add(TEXT("- Use actor/Data Layer/tag scans to exclude dynamic, gameplay-critical, destructible, or interactive scene actors."));
	ReportLines.Add(TEXT("- This report does not create Texture2DArray, property textures, UV index data, or generated proxy meshes yet."));

	FString SavedReportPath;
	FString SharedReportPath;
	const bool bSaved = DevKitEditorCommandletReports::SaveReportLines(MaterialBatchAuditReportFileName, ReportLines, SavedReportPath, SharedReportPath);
	if (!bSaved)
	{
		UE_LOG(LogTemp, Error, TEXT("MaterialBatchAudit could not save reports."));
		return 1;
	}

	UE_LOG(LogTemp, Display, TEXT("MaterialBatchAudit wrote: %s"), *SavedReportPath);
	UE_LOG(LogTemp, Display, TEXT("MaterialBatchAudit wrote shared report: %s"), *SharedReportPath);
	return 0;
}
