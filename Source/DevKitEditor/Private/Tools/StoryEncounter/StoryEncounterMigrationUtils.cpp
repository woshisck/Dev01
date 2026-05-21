#include "Tools/StoryEncounter/StoryEncounterMigrationUtils.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "Components/BoxComponent.h"
#include "Editor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Factories/DataAssetFactory.h"
#include "GenericGraphAssetEditor/AssetGraphSchema_GenericGraph.h"
#include "GenericGraphAssetEditor/EdGraph_GenericGraph.h"
#include "GenericGraphAssetEditor/EdNode_GenericGraphNode.h"
#include "GenericGraphFactory.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "LevelFlow/LevelEventTrigger.h"
#include "LevelFlow/LevelFlowAsset.h"
#include "Misc/PackageName.h"
#include "ObjectTools.h"
#include "ScopedTransaction.h"
#include "Story/Encounter/StoryEncounterGraph.h"
#include "Story/Encounter/StoryEncounterGraphNode.h"
#include "Story/Encounter/StoryEncounterPointDataAsset.h"
#include "Story/Encounter/StoryEncounterTrigger.h"

#define LOCTEXT_NAMESPACE "StoryEncounterMigrationUtils"

namespace
{
FString SanitizeAssetSegment(const FString& RawName, const FString& Fallback)
{
	FString CleanName = ObjectTools::SanitizeObjectName(RawName);
	CleanName.TrimStartAndEndInline();
	CleanName.ReplaceInline(TEXT(" "), TEXT("_"));
	return CleanName.IsEmpty() ? Fallback : CleanName;
}

FName MakeUniqueName(TSet<FName>& UsedNames, const FString& BaseName)
{
	FString CleanBase = SanitizeAssetSegment(BaseName, TEXT("Node"));
	FName Candidate(*CleanBase);
	int32 Suffix = 2;
	while (UsedNames.Contains(Candidate))
	{
		Candidate = FName(*FString::Printf(TEXT("%s_%d"), *CleanBase, Suffix++));
	}
	UsedNames.Add(Candidate);
	return Candidate;
}

UObject* CreateDataAsset(UClass* AssetClass, const FString& BasePackageName)
{
	if (!AssetClass)
	{
		return nullptr;
	}

	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	FString UniquePackageName;
	FString UniqueAssetName;
	AssetTools.CreateUniqueAssetName(BasePackageName, TEXT(""), UniquePackageName, UniqueAssetName);

	UDataAssetFactory* Factory = NewObject<UDataAssetFactory>();
	Factory->DataAssetClass = AssetClass;
	UObject* Asset = AssetTools.CreateAsset(
		UniqueAssetName,
		FPackageName::GetLongPackagePath(UniquePackageName),
		AssetClass,
		Factory);

	if (Asset)
	{
		FAssetRegistryModule::AssetCreated(Asset);
		Asset->MarkPackageDirty();
	}
	return Asset;
}

UStoryEncounterGraph* CreateEncounterGraphAsset(const FString& BasePackageName, FName EncounterId, const FString& MapName)
{
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	FString UniquePackageName;
	FString UniqueAssetName;
	AssetTools.CreateUniqueAssetName(BasePackageName, TEXT(""), UniquePackageName, UniqueAssetName);

	UGenericGraphFactory* Factory = NewObject<UGenericGraphFactory>();
	Factory->GenericGraphClass = UStoryEncounterGraph::StaticClass();
	UStoryEncounterGraph* EncounterGraph = Cast<UStoryEncounterGraph>(AssetTools.CreateAsset(
		UniqueAssetName,
		FPackageName::GetLongPackagePath(UniquePackageName),
		UStoryEncounterGraph::StaticClass(),
		Factory));

	if (!EncounterGraph)
	{
		return nullptr;
	}

	EncounterGraph->Modify();
	EncounterGraph->EncounterId = EncounterId;
	EncounterGraph->DisplayName = FText::FromString(FString::Printf(TEXT("%s 自动迁移"), *MapName));
	EncounterGraph->Description = FText::FromString(TEXT("由旧 LevelEventTrigger 自动迁移生成。每个节点绑定一个剧情点 DA，运行时仍播放原 LevelFlow。"));

#if WITH_EDITORONLY_DATA
	EncounterGraph->EdGraph = CastChecked<UEdGraph_GenericGraph>(
		FBlueprintEditorUtils::CreateNewGraph(
			EncounterGraph,
			NAME_None,
			UEdGraph_GenericGraph::StaticClass(),
			UAssetGraphSchema_GenericGraph::StaticClass()));
	EncounterGraph->EdGraph->bAllowDeletion = false;

	if (const UEdGraphSchema* Schema = EncounterGraph->EdGraph->GetSchema())
	{
		Schema->CreateDefaultNodesForGraph(*EncounterGraph->EdGraph);
	}
#endif

	FAssetRegistryModule::AssetCreated(EncounterGraph);
	EncounterGraph->MarkPackageDirty();
	return EncounterGraph;
}

void AddGraphNode(UStoryEncounterGraph* EncounterGraph, UStoryEncounterPointDA* EncounterPoint, int32 NodeIndex)
{
	if (!EncounterGraph || !EncounterPoint || !EncounterGraph->EdGraph)
	{
		return;
	}

	UEdGraph_GenericGraph* EdGraph = Cast<UEdGraph_GenericGraph>(EncounterGraph->EdGraph);
	if (!EdGraph)
	{
		return;
	}

	EncounterGraph->Modify();
	EdGraph->Modify();

	UEdNode_GenericGraphNode* EdNode = NewObject<UEdNode_GenericGraphNode>(EdGraph);
	UStoryEncounterGraphNode* GraphNode = NewObject<UStoryEncounterGraphNode>(EdNode, EncounterGraph->NodeType);
	GraphNode->Point = EncounterPoint;
	GraphNode->FallbackEncounterId = EncounterPoint->EncounterId;
	GraphNode->FallbackNodeId = EncounterPoint->GetStableNodeId();
	GraphNode->FallbackTitle = EncounterPoint->DisplayName;
	GraphNode->Graph = EncounterGraph;
	GraphNode->SetFlags(RF_Transactional);

	EdNode->GenericGraphNode = GraphNode;
	EdGraph->AddNode(EdNode, true, false);
	EdNode->CreateNewGuid();
	EdNode->PostPlacedNewNode();
	EdNode->AllocateDefaultPins();
	EdNode->NodePosX = (NodeIndex % 4) * 360;
	EdNode->NodePosY = (NodeIndex / 4) * 220;
	EdNode->SetFlags(RF_Transactional);

	EdGraph->RebuildGenericGraph();
	EdGraph->NotifyGraphChanged();
	EncounterGraph->MarkPackageDirty();
}

void CopyTriggerShape(ALevelEventTrigger* OldTrigger, AStoryEncounterTrigger* NewTrigger)
{
	if (!OldTrigger || !NewTrigger)
	{
		return;
	}

	UBoxComponent* OldBox = OldTrigger->GetTriggerVolume();
	UBoxComponent* NewBox = NewTrigger->GetTriggerVolume();
	if (!OldBox || !NewBox)
	{
		return;
	}

	NewBox->Modify();
	NewBox->SetBoxExtent(OldBox->GetUnscaledBoxExtent());
	NewBox->SetRelativeTransform(OldBox->GetRelativeTransform());
	NewBox->SetCollisionProfileName(OldBox->GetCollisionProfileName());
	NewBox->SetGenerateOverlapEvents(OldBox->GetGenerateOverlapEvents());
}
}

FText FStoryEncounterMigrationResult::ToStatusText() const
{
	return FText::FromString(FString::Printf(
		TEXT("旧触发器迁移完成：扫描 %d 个，迁移 %d 个，跳过 %d 个，失败 %d 个。%s"),
		ScannedTriggers,
		MigratedTriggers,
		SkippedTriggers,
		FailedTriggers,
		GraphPath.IsEmpty() ? TEXT("") : *FString::Printf(TEXT("流程图：%s"), *GraphPath)));
}

FStoryEncounterMigrationResult FStoryEncounterMigrationUtils::MigrateCurrentLevelTriggers()
{
	FStoryEncounterMigrationResult Result;

	if (!GEditor)
	{
		Result.Messages.Add(TEXT("GEditor 不可用。"));
		Result.FailedTriggers++;
		return Result;
	}

	UWorld* World = GEditor->GetEditorWorldContext().World();
	if (!World)
	{
		Result.Messages.Add(TEXT("没有找到当前编辑器关卡。"));
		Result.FailedTriggers++;
		return Result;
	}

	TArray<ALevelEventTrigger*> TriggersToMigrate;
	for (TActorIterator<ALevelEventTrigger> It(World); It; ++It)
	{
		ALevelEventTrigger* Trigger = *It;
		Result.ScannedTriggers++;
		if (!Trigger || Trigger->IsPendingKillPending())
		{
			Result.SkippedTriggers++;
			continue;
		}

		if (!Trigger->GetLevelFlow())
		{
			Result.SkippedTriggers++;
			Result.Messages.Add(FString::Printf(TEXT("%s 没有绑定 LevelFlow，已跳过。"), *Trigger->GetActorLabel()));
			continue;
		}

		TriggersToMigrate.Add(Trigger);
	}

	if (TriggersToMigrate.Num() == 0)
	{
		Result.Messages.Add(TEXT("当前关卡没有可迁移的旧 LevelEventTrigger。"));
		return Result;
	}

	const FString MapName = FPackageName::GetShortName(World->GetOutermost()->GetName());
	const FString CleanMapName = SanitizeAssetSegment(MapName, TEXT("CurrentMap"));
	const FName EncounterId(*FString::Printf(TEXT("EM_%s_Migrated"), *CleanMapName));

	const FScopedTransaction Transaction(LOCTEXT("MigrateLegacyLevelEventTriggers", "Migrate Legacy LevelEventTriggers"));
	World->Modify();

	UStoryEncounterGraph* EncounterGraph = CreateEncounterGraphAsset(
		FString::Printf(TEXT("/Game/Story/Encounters/Migrated/EG_%s_Migrated"), *CleanMapName),
		EncounterId,
		MapName);
	if (!EncounterGraph)
	{
		Result.FailedTriggers += TriggersToMigrate.Num();
		Result.Messages.Add(TEXT("创建迁移流程图失败，未改动旧触发器。"));
		return Result;
	}
	Result.GraphPath = EncounterGraph->GetPathName();

	TSet<FName> UsedNodeIds;
	int32 GraphNodeIndex = 0;
	for (ALevelEventTrigger* OldTrigger : TriggersToMigrate)
	{
		if (!OldTrigger || !OldTrigger->GetLevelFlow())
		{
			Result.SkippedTriggers++;
			continue;
		}

		const FString ActorLabel = OldTrigger->GetActorLabel();
		const FString CleanActorName = SanitizeAssetSegment(ActorLabel, OldTrigger->GetName());
		const FName NodeId = MakeUniqueName(UsedNodeIds, CleanActorName);
		const FString PointBasePackage = FString::Printf(
			TEXT("/Game/Story/EncounterPoints/Migrated/%s/EP_%s_%s"),
			*CleanMapName,
			*CleanMapName,
			*NodeId.ToString());

		UStoryEncounterPointDA* EncounterPoint = Cast<UStoryEncounterPointDA>(
			CreateDataAsset(UStoryEncounterPointDA::StaticClass(), PointBasePackage));
		if (!EncounterPoint)
		{
			Result.FailedTriggers++;
			Result.Messages.Add(FString::Printf(TEXT("%s 创建剧情点 DA 失败。"), *ActorLabel));
			continue;
		}

		EncounterPoint->Modify();
		EncounterPoint->EncounterId = EncounterId;
		EncounterPoint->NodeId = NodeId;
		EncounterPoint->DisplayName = FText::FromString(FString::Printf(TEXT("迁移：%s"), *ActorLabel));
		EncounterPoint->Kind = EStoryEncounterNodeKind::Area;
		EncounterPoint->PlayerFacingEvent = FText::FromString(FString::Printf(
			TEXT("由旧 LevelEventTrigger 自动迁移：%s"),
			*GetNameSafe(OldTrigger->GetLevelFlow())));
		EncounterPoint->FirePolicy = OldTrigger->ShouldTriggerOnce()
			? EStoryEncounterFirePolicy::Once
			: EStoryEncounterFirePolicy::Repeat;
		EncounterPoint->PlacementLevel = FName(*MapName);
		EncounterPoint->PlacementName = FName(*ActorLabel);
		EncounterPoint->EditorPosition = FVector2D((GraphNodeIndex % 4) * 360.f, (GraphNodeIndex / 4) * 220.f);

		FStoryEncounterAction Action;
		Action.Kind = EStoryEncounterActionKind::PlayLevelFlow;
		Action.Title = FText::FromString(TEXT("播放旧 Flow"));
		Action.Body = FText::FromString(TEXT("自动迁移保留的旧版教程/演出流程。"));
		Action.LevelFlow = OldTrigger->GetLevelFlow();
		EncounterPoint->Actions.Add(Action);
		EncounterPoint->MarkPackageDirty();

		FActorSpawnParameters SpawnParameters;
		SpawnParameters.ObjectFlags = RF_Transactional;
		SpawnParameters.Name = MakeUniqueObjectName(
			World,
			AStoryEncounterTrigger::StaticClass(),
			FName(*FString::Printf(TEXT("%s_StoryEncounter"), *CleanActorName)));

		AStoryEncounterTrigger* NewTrigger = World->SpawnActor<AStoryEncounterTrigger>(
			AStoryEncounterTrigger::StaticClass(),
			OldTrigger->GetActorTransform(),
			SpawnParameters);
		if (!NewTrigger)
		{
			Result.FailedTriggers++;
			Result.Messages.Add(FString::Printf(TEXT("%s 创建新 StoryEncounterTrigger 失败。"), *ActorLabel));
			continue;
		}

		NewTrigger->Modify();
		NewTrigger->SetActorLabel(FString::Printf(TEXT("%s_StoryEncounter"), *ActorLabel));
		NewTrigger->EncounterPoint = EncounterPoint;
		CopyTriggerShape(OldTrigger, NewTrigger);
#if WITH_EDITOR
		NewTrigger->SetFolderPath(OldTrigger->GetFolderPath());
#endif
		NewTrigger->MarkPackageDirty();

		AddGraphNode(EncounterGraph, EncounterPoint, GraphNodeIndex++);

		World->EditorDestroyActor(OldTrigger, true);
		Result.MigratedTriggers++;
	}

	World->MarkPackageDirty();
	return Result;
}

#undef LOCTEXT_NAMESPACE
