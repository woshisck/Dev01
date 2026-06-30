#include "MaterialBatch/EnvBatchAutoTagCommandlet.h"

#include "Commandlets/CommandletReportUtils.h"
#include "Components/StaticMeshComponent.h"
#include "Editor.h"
#include "Engine/Level.h"
#include "Engine/LevelStreaming.h"
#include "Engine/World.h"
#include "FileHelpers.h"
#include "GameFramework/Actor.h"
#include "MaterialBatch/MaterialBatchAuditHelpers.h"
#include "MaterialBatch/MaterialBatchCandidateRules.h"
#include "Misc/Parse.h"

namespace
{
const TCHAR* EnvBatchAutoTagReportFileName = TEXT("EnvBatchAutoTagReport.md");

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

bool HasEnvBatchTag(const AActor& Actor)
{
	for (const FName& Tag : Actor.Tags)
	{
		if (Tag.ToString().StartsWith(TEXT("EnvBatch.")))
		{
			return true;
		}
	}
	return false;
}

FString JoinActorTags(const TArray<FName>& Tags)
{
	TArray<FString> Values;
	Values.Reserve(Tags.Num());
	for (const FName& Tag : Tags)
	{
		Values.Add(Tag.ToString());
	}
	Values.Sort();
	return Values.IsEmpty() ? TEXT("") : FString::Join(Values, TEXT(", "));
}

struct FAutoTagActorScanResult
{
	FString ActorName;
	FString LevelName;
	FString ExistingTags;
	int32 StaticMeshComponentCount = 0;
	int32 EligibleComponentCount = 0;
	bool bAlreadyTagged = false;
	bool bWouldApply = false;
	bool bApplied = false;
	FString Status;
	FString Reason;
};

FMaterialBatchComponentScanInput BuildScanInput(const AActor& Actor, const UStaticMeshComponent& Component)
{
	FMaterialBatchComponentScanInput Input;
	Input.bIsStaticMeshComponent = Component.GetStaticMesh() != nullptr;
	Input.bHasStaticMobility = Component.Mobility == EComponentMobility::Static;
	if (const UStaticMesh* StaticMesh = Component.GetStaticMesh())
	{
		Input.MaterialSlotCount = StaticMesh->GetStaticMaterials().Num();
		Input.LodCount = StaticMesh->GetNumLODs();
	}

	TArray<FName> CombinedTags = Actor.Tags;
	CombinedTags.Append(Component.ComponentTags);
	return FMaterialBatchCandidateRules::BuildInputFromTags(Input, CombinedTags);
}

FAutoTagActorScanResult ScanActorForAutoTag(AActor& Actor, bool bIncludeAlreadyTagged)
{
	FAutoTagActorScanResult Result;
	Result.ActorName = Actor.GetActorNameOrLabel();
	Result.LevelName = Actor.GetLevel() ? Actor.GetLevel()->GetOuter()->GetName() : TEXT("");
	Result.ExistingTags = JoinActorTags(Actor.Tags);
	Result.bAlreadyTagged = HasEnvBatchTag(Actor);

	if (Result.bAlreadyTagged && !bIncludeAlreadyTagged)
	{
		Result.Status = TEXT("Skipped");
		Result.Reason = TEXT("AlreadyHasEnvBatchTag");
		return Result;
	}

	TArray<UStaticMeshComponent*> Components;
	Actor.GetComponents<UStaticMeshComponent>(Components);
	Result.StaticMeshComponentCount = Components.Num();
	if (Components.IsEmpty())
	{
		Result.Status = TEXT("Rejected");
		Result.Reason = TEXT("NoStaticMeshComponents");
		return Result;
	}

	TArray<FString> RejectReasons;
	for (UStaticMeshComponent* Component : Components)
	{
		if (!Component)
		{
			continue;
		}

		const FMaterialBatchComponentScanInput Input = BuildScanInput(Actor, *Component);
		const FMaterialBatchCandidateDecision Decision = FMaterialBatchCandidateRules::ClassifyComponent(Input);
		if (Decision.bEligible)
		{
			++Result.EligibleComponentCount;
		}
		else
		{
			RejectReasons.AddUnique(FMaterialBatchCandidateRules::RejectReasonToString(Decision.Reason));
		}
	}

	if (Result.EligibleComponentCount <= 0)
	{
		Result.Status = TEXT("Rejected");
		Result.Reason = RejectReasons.IsEmpty() ? TEXT("NoEligibleComponents") : FString::Join(RejectReasons, TEXT(", "));
		return Result;
	}

	Result.bWouldApply = true;
	Result.Status = TEXT("Candidate");
	Result.Reason = TEXT("EligibleStaticMeshActor");
	return Result;
}

void LoadAllStreamingLevels(UWorld& World)
{
	for (ULevelStreaming* StreamingLevel : World.GetStreamingLevels())
	{
		if (StreamingLevel)
		{
			StreamingLevel->SetShouldBeLoaded(true);
			StreamingLevel->SetShouldBeVisible(true);
		}
	}
	World.FlushLevelStreaming(EFlushLevelStreamingType::Full);
}
}

UEnvBatchAutoTagCommandlet::UEnvBatchAutoTagCommandlet()
{
	IsClient = false;
	IsEditor = true;
	IsServer = false;
	LogToConsole = true;
}

int32 UEnvBatchAutoTagCommandlet::Main(const FString& Params)
{
	const FString MapPath = GetParamValue(Params, TEXT("Map="), TEXT(""));
	const FString Cluster = GetParamValue(Params, TEXT("Cluster="), TEXT("Prison_S_01"));
	const FString SourceTag = GetParamValue(Params, TEXT("SourceTag="), FString::Printf(TEXT("EnvBatch.Source.%s"), *Cluster));
	const int32 MaxActors = FMath::Max(0, GetParamInt(Params, TEXT("MaxActors="), 2000));
	const int32 MaxApplyActors = FMath::Max(0, GetParamInt(Params, TEXT("MaxApplyActors="), 0));
	const bool bApply = FParse::Param(*Params, TEXT("Apply"));
	const bool bIncludeAlreadyTagged = FParse::Param(*Params, TEXT("IncludeAlreadyTagged"));

	if (MapPath.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("EnvBatchAutoTag requires -Map=/Game/..."));
		return 1;
	}
	if (!SourceTag.StartsWith(TEXT("EnvBatch.Source.")))
	{
		UE_LOG(LogTemp, Error, TEXT("EnvBatchAutoTag SourceTag must start with EnvBatch.Source.: %s"), *SourceTag);
		return 1;
	}

	const FString MapFilename = FMaterialBatchAuditHelpers::ResolveMapFilename(MapPath);
	UWorld* World = UEditorLoadingAndSavingUtils::LoadMap(MapFilename);
	if (!World)
	{
		UE_LOG(LogTemp, Error, TEXT("EnvBatchAutoTag could not load map: %s"), *MapPath);
		return 1;
	}

	LoadAllStreamingLevels(*World);

	TArray<FAutoTagActorScanResult> Results;
	TArray<UPackage*> PackagesToSave;
	int32 ActorsVisited = 0;
	int32 CandidateActors = 0;
	int32 AppliedActors = 0;
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
			if (MaxActors > 0 && ActorsVisited >= MaxActors)
			{
				break;
			}
			++ActorsVisited;

			FAutoTagActorScanResult Result = ScanActorForAutoTag(*Actor, bIncludeAlreadyTagged);
			if (Result.bWouldApply)
			{
				++CandidateActors;
				if (bApply && (MaxApplyActors <= 0 || AppliedActors < MaxApplyActors))
				{
					Actor->Modify();
					Actor->Tags.RemoveAll([](const FName& ExistingTag)
					{
						const FString TagString = ExistingTag.ToString();
						return TagString.StartsWith(TEXT("EnvBatch.Source."))
							|| TagString.StartsWith(TEXT("EnvBatch.Proxy."))
							|| TagString.StartsWith(TEXT("EnvBatch.Baked."));
					});
					Actor->Tags.AddUnique(FName(*SourceTag));
					Actor->MarkPackageDirty();
					if (UPackage* Package = Actor->GetOutermost())
					{
						PackagesToSave.AddUnique(Package);
					}
					Result.bApplied = true;
					Result.Status = TEXT("Applied");
					Result.Reason = SourceTag;
					++AppliedActors;
				}
			}
			Results.Add(Result);
		}

		if (MaxActors > 0 && ActorsVisited >= MaxActors)
		{
			break;
		}
	}

	bool bSavedPackages = false;
	if (bApply && !PackagesToSave.IsEmpty())
	{
		bSavedPackages = UEditorLoadingAndSavingUtils::SavePackages(PackagesToSave, true);
		if (!bSavedPackages)
		{
			UE_LOG(LogTemp, Error, TEXT("EnvBatchAutoTag failed to save one or more level packages."));
		}
	}

	TArray<FString> Lines;
	Lines.Add(TEXT("# EnvBatch Auto Tag Report"));
	Lines.Add(TEXT(""));
	Lines.Add(FString::Printf(TEXT("- Map: `%s`"), *MapPath));
	Lines.Add(FString::Printf(TEXT("- Cluster: `%s`"), *Cluster));
	Lines.Add(FString::Printf(TEXT("- Source tag: `%s`"), *SourceTag));
	Lines.Add(FString::Printf(TEXT("- Apply requested: %s"), bApply ? TEXT("True") : TEXT("False")));
	Lines.Add(FString::Printf(TEXT("- Include already tagged: %s"), bIncludeAlreadyTagged ? TEXT("True") : TEXT("False")));
	Lines.Add(FString::Printf(TEXT("- Max actors: %d"), MaxActors));
	Lines.Add(FString::Printf(TEXT("- Max apply actors: %d"), MaxApplyActors));
	Lines.Add(FString::Printf(TEXT("- Actors visited: %d"), ActorsVisited));
	Lines.Add(FString::Printf(TEXT("- Candidate actors: %d"), CandidateActors));
	Lines.Add(FString::Printf(TEXT("- Applied actors: %d"), AppliedActors));
	Lines.Add(FString::Printf(TEXT("- Saved packages: %s"), bSavedPackages ? TEXT("True") : TEXT("False")));
	Lines.Add(TEXT(""));
	Lines.Add(TEXT("## Actor Results"));
	Lines.Add(TEXT(""));
	Lines.Add(TEXT("| Actor | StreamingLevel | Status | Reason | StaticMeshComponents | EligibleComponents | ExistingTags |"));
	Lines.Add(TEXT("| --- | --- | --- | --- | ---: | ---: | --- |"));
	for (const FAutoTagActorScanResult& Result : Results)
	{
		Lines.Add(FString::Printf(
			TEXT("| `%s` | `%s` | %s | %s | %d | %d | `%s` |"),
			*Result.ActorName,
			*Result.LevelName,
			*Result.Status,
			*Result.Reason,
			Result.StaticMeshComponentCount,
			Result.EligibleComponentCount,
			*Result.ExistingTags));
	}
	Lines.Add(TEXT(""));
	Lines.Add(TEXT("## Acceptance Notes"));
	Lines.Add(TEXT(""));
	Lines.Add(TEXT("- Report-only mode does not modify the map."));
	Lines.Add(TEXT("- `-Apply` writes `EnvBatch.Source.<Cluster>` to eligible static mesh actors and saves the affected level packages."));
	Lines.Add(TEXT("- Existing `EnvBatch.Source.*`, `EnvBatch.Proxy.*`, and `EnvBatch.Baked.*` tags are mutually exclusive and are replaced only for actors selected for apply."));
	Lines.Add(TEXT("- StreamingLevel ownership is reported as the scene-layer evidence for the current non-World-Partition workflow."));

	FString SavedReportPath;
	FString SharedReportPath;
	if (!DevKitEditorCommandletReports::SaveReportLines(EnvBatchAutoTagReportFileName, Lines, SavedReportPath, SharedReportPath))
	{
		UE_LOG(LogTemp, Error, TEXT("EnvBatchAutoTag could not save report."));
		return 1;
	}

	UE_LOG(LogTemp, Display, TEXT("EnvBatchAutoTag wrote: %s"), *SavedReportPath);
	UE_LOG(LogTemp, Display, TEXT("EnvBatchAutoTag wrote shared report: %s"), *SharedReportPath);

	if (bApply && CandidateActors > 0 && AppliedActors <= 0)
	{
		return 1;
	}
	if (bApply && !PackagesToSave.IsEmpty() && !bSavedPackages)
	{
		return 1;
	}
	return 0;
}
