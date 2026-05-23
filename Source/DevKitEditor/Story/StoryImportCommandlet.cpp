#include "Story/StoryImportCommandlet.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "FileHelpers.h"
#include "GenericGraphAssetEditor/AssetGraphSchema_GenericGraph.h"
#include "GenericGraphAssetEditor/EdGraph_GenericGraph.h"
#include "GenericGraphAssetEditor/EdNode_GenericGraphNode.h"
#include "JsonObjectConverter.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "LevelFlow/LevelFlowAsset.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Story/Encounter/StoryEncounterGraph.h"
#include "Story/Encounter/StoryEncounterGraphEdge.h"
#include "Story/Encounter/StoryEncounterGraphNode.h"
#include "Story/Encounter/StoryEncounterPointDataAsset.h"
#include "UI/GameDialogWidget.h"
#include "Engine/Texture2D.h"
#include "UObject/Package.h"

namespace StoryImport
{
const FString DefaultManifestPath = TEXT("Docs/StoryPipeline/StoryImportManifest.json");
const FString GraphRoot = TEXT("/Game/Story/Encounters");
const FString PointRoot = TEXT("/Game/Story/EncounterPoints");

FString ToObjectPath(const FString& PackagePath)
{
	return PackagePath + TEXT(".") + FPackageName::GetLongPackageAssetName(PackagePath);
}

bool PackageExists(const FString& PackagePath)
{
	FString ExistingPackageFile;
	return FPackageName::DoesPackageExist(PackagePath, &ExistingPackageFile);
}

template <typename T>
T* LoadOrCreateAsset(const FString& PackagePath, TArray<UPackage*>& DirtyPackages, bool& bOutCreated)
{
	bOutCreated = false;
	if (T* Existing = Cast<T>(StaticLoadObject(T::StaticClass(), nullptr, *ToObjectPath(PackagePath))))
	{
		return Existing;
	}

	if (PackageExists(PackagePath))
	{
		return Cast<T>(StaticLoadObject(T::StaticClass(), nullptr, *ToObjectPath(PackagePath)));
	}

	UPackage* Package = CreatePackage(*PackagePath);
	if (!Package)
	{
		return nullptr;
	}

	T* Asset = NewObject<T>(
		Package,
		*FPackageName::GetLongPackageAssetName(PackagePath),
		RF_Public | RF_Standalone | RF_Transactional);
	if (Asset)
	{
		FAssetRegistryModule::AssetCreated(Asset);
		Asset->MarkPackageDirty();
		DirtyPackages.AddUnique(Package);
		bOutCreated = true;
	}
	return Asset;
}

FString GetString(const TSharedPtr<FJsonObject>& Object, const TCHAR* Field, const FString& Default = FString())
{
	if (!Object.IsValid())
	{
		return Default;
	}
	FString Value;
	return Object->TryGetStringField(Field, Value) ? Value : Default;
}

int32 GetInt(const TSharedPtr<FJsonObject>& Object, const TCHAR* Field, int32 Default = 0)
{
	if (!Object.IsValid())
	{
		return Default;
	}
	double Value = 0.0;
	return Object->TryGetNumberField(Field, Value) ? FMath::RoundToInt(Value) : Default;
}

bool GetBool(const TSharedPtr<FJsonObject>& Object, const TCHAR* Field, bool bDefault = false)
{
	if (!Object.IsValid())
	{
		return bDefault;
	}
	bool bValue = false;
	if (Object->TryGetBoolField(Field, bValue))
	{
		return bValue;
	}
	FString StringValue;
	if (Object->TryGetStringField(Field, StringValue))
	{
		return StringValue.Equals(TEXT("true"), ESearchCase::IgnoreCase);
	}
	return bDefault;
}

TArray<TSharedPtr<FJsonValue>> GetArray(const TSharedPtr<FJsonObject>& Object, const TCHAR* Field)
{
	if (!Object.IsValid())
	{
		return {};
	}
	const TArray<TSharedPtr<FJsonValue>>* Array = nullptr;
	return Object->TryGetArrayField(Field, Array) ? *Array : TArray<TSharedPtr<FJsonValue>>();
}

TSharedPtr<FJsonObject> GetObject(const TSharedPtr<FJsonObject>& Object, const TCHAR* Field)
{
	if (!Object.IsValid())
	{
		return nullptr;
	}
	const TSharedPtr<FJsonObject>* Child = nullptr;
	return Object->TryGetObjectField(Field, Child) ? *Child : nullptr;
}

FString ToLoadableObjectPath(const FString& AssetPath)
{
	if (AssetPath.IsEmpty() || AssetPath.Contains(TEXT(".")))
	{
		return AssetPath;
	}
	return ToObjectPath(AssetPath);
}

FTutorialPage ParseTutorialPage(const TSharedPtr<FJsonObject>& Object)
{
	FTutorialPage Page;
	Page.Title = FText::FromString(GetString(Object, TEXT("title")));
	Page.Body = FText::FromString(GetString(Object, TEXT("body")));
	Page.SubText = FText::FromString(GetString(Object, TEXT("subText")));

	const FString IllustrationPath = GetString(Object, TEXT("illustration"));
	if (!IllustrationPath.IsEmpty())
	{
		Page.Illustration = Cast<UTexture2D>(StaticLoadObject(
			UTexture2D::StaticClass(),
			nullptr,
			*ToLoadableObjectPath(IllustrationPath)));
	}
	return Page;
}

EStoryEncounterNodeKind ParseNodeKind(const FString& Value)
{
	if (Value.Equals(TEXT("Area"), ESearchCase::IgnoreCase)) return EStoryEncounterNodeKind::Area;
	if (Value.Equals(TEXT("Object"), ESearchCase::IgnoreCase)) return EStoryEncounterNodeKind::Object;
	if (Value.Equals(TEXT("NPC"), ESearchCase::IgnoreCase)) return EStoryEncounterNodeKind::NPC;
	if (Value.Equals(TEXT("Death"), ESearchCase::IgnoreCase)) return EStoryEncounterNodeKind::Death;
	if (Value.Equals(TEXT("Feature"), ESearchCase::IgnoreCase)) return EStoryEncounterNodeKind::Feature;
	return EStoryEncounterNodeKind::System;
}

EStoryEncounterFirePolicy ParseFirePolicy(const FString& Value)
{
	if (Value.Equals(TEXT("Repeat"), ESearchCase::IgnoreCase)) return EStoryEncounterFirePolicy::Repeat;
	if (Value.Equals(TEXT("OncePerRun"), ESearchCase::IgnoreCase)) return EStoryEncounterFirePolicy::OncePerRun;
	return EStoryEncounterFirePolicy::Once;
}

EStoryEncounterConditionKind ParseConditionKind(const FString& Value)
{
	if (Value.Equals(TEXT("ProgressMissing"), ESearchCase::IgnoreCase)) return EStoryEncounterConditionKind::ProgressMissing;
	if (Value.Equals(TEXT("ProgressCompleted"), ESearchCase::IgnoreCase)) return EStoryEncounterConditionKind::ProgressCompleted;
	if (Value.Equals(TEXT("RunCountAtLeast"), ESearchCase::IgnoreCase)) return EStoryEncounterConditionKind::RunCountAtLeast;
	if (Value.Equals(TEXT("FeatureUnlocked"), ESearchCase::IgnoreCase)) return EStoryEncounterConditionKind::FeatureUnlocked;
	return EStoryEncounterConditionKind::None;
}

EStoryEncounterActionKind ParseActionKind(const FString& Value)
{
	if (Value.Equals(TEXT("Dialogue"), ESearchCase::IgnoreCase)) return EStoryEncounterActionKind::Dialogue;
	if (Value.Equals(TEXT("RecordProgress"), ESearchCase::IgnoreCase)) return EStoryEncounterActionKind::RecordProgress;
	if (Value.Equals(TEXT("UnlockFeature"), ESearchCase::IgnoreCase)) return EStoryEncounterActionKind::UnlockFeature;
	if (Value.Equals(TEXT("SetQuestObjective"), ESearchCase::IgnoreCase)) return EStoryEncounterActionKind::SetQuestObjective;
	if (Value.Equals(TEXT("TeleportToNode"), ESearchCase::IgnoreCase)) return EStoryEncounterActionKind::TeleportToNode;
	if (Value.Equals(TEXT("PlayLevelFlow"), ESearchCase::IgnoreCase)) return EStoryEncounterActionKind::PlayLevelFlow;
	if (Value.Equals(TEXT("TutorialPopup"), ESearchCase::IgnoreCase)) return EStoryEncounterActionKind::TutorialPopup;
	return EStoryEncounterActionKind::WeakHint;
}

FStoryEncounterCondition ParseCondition(const TSharedPtr<FJsonObject>& Object)
{
	FStoryEncounterCondition Condition;
	Condition.Kind = ParseConditionKind(GetString(Object, TEXT("kind")));
	Condition.ProgressKey = FName(*GetString(Object, TEXT("progressKey")));
	Condition.ProgressLabel = FText::FromString(GetString(Object, TEXT("progressLabel")));
	const FString FeatureTag = GetString(Object, TEXT("featureTag"));
	if (!FeatureTag.IsEmpty())
	{
		Condition.FeatureTag = FGameplayTag::RequestGameplayTag(FName(*FeatureTag), false);
	}
	Condition.RunCount = GetInt(Object, TEXT("runCount"));
	return Condition;
}

FStoryEncounterAction ParseAction(const TSharedPtr<FJsonObject>& Object)
{
	FStoryEncounterAction Action;
	Action.Kind = ParseActionKind(GetString(Object, TEXT("type")));
	Action.ActionId = FName(*GetString(Object, TEXT("actionId")));
	Action.ReuseKey = FName(*GetString(Object, TEXT("reuseKey")));
	Action.Title = FText::FromString(GetString(Object, TEXT("title")));
	Action.Body = FText::FromString(GetString(Object, TEXT("body")));
	const TSharedPtr<FJsonObject> InputTextVariants = GetObject(Object, TEXT("inputTextVariants"));
	const FString KeyboardMouseBody = GetString(
		InputTextVariants,
		TEXT("keyboardMouse"),
		GetString(Object, TEXT("keyboardMouseBody")));
	const FString GamepadBody = GetString(
		InputTextVariants,
		TEXT("gamepad"),
		GetString(Object, TEXT("gamepadBody")));
	Action.bUseInputTextVariants = GetBool(Object, TEXT("useInputTextVariants"), !KeyboardMouseBody.IsEmpty() || !GamepadBody.IsEmpty());
	Action.KeyboardMouseBody = FText::FromString(KeyboardMouseBody);
	Action.GamepadBody = FText::FromString(GamepadBody);
	Action.TutorialEventId = FName(*GetString(Object, TEXT("tutorialEventId")));
	Action.TutorialPages.Reset();
	for (const TSharedPtr<FJsonValue>& PageValue : GetArray(Object, TEXT("tutorialPages")))
	{
		if (const TSharedPtr<FJsonObject>* PageObject = nullptr; PageValue->TryGetObject(PageObject))
		{
			Action.TutorialPages.Add(ParseTutorialPage(*PageObject));
		}
	}
	Action.bPauseGame = !GetString(Object, TEXT("pauseGame")).Equals(TEXT("false"), ESearchCase::IgnoreCase);
	Action.ProgressKey = FName(*GetString(Object, TEXT("progressKey")));
	Action.ProgressLabel = FText::FromString(GetString(Object, TEXT("progressLabel")));
	const FString FeatureTag = GetString(Object, TEXT("featureTag"));
	if (!FeatureTag.IsEmpty())
	{
		Action.FeatureTag = FGameplayTag::RequestGameplayTag(FName(*FeatureTag), false);
	}
	const FString QuestTaskTag = GetString(Object, TEXT("questTaskTag"));
	if (!QuestTaskTag.IsEmpty())
	{
		Action.QuestTaskTag = FGameplayTag::RequestGameplayTag(FName(*QuestTaskTag), false);
	}
	Action.TargetNodeId = FName(*GetString(Object, TEXT("targetNodeId")));
	const FString LevelFlowPath = GetString(Object, TEXT("levelFlow"));
	if (!LevelFlowPath.IsEmpty())
	{
		Action.LevelFlow = Cast<ULevelFlowAsset>(StaticLoadObject(ULevelFlowAsset::StaticClass(), nullptr, *LevelFlowPath));
	}
	return Action;
}

FString SanitizeAssetName(const FString& Value)
{
	FString Out = Value;
	for (TCHAR& Char : Out)
	{
		if (!FChar::IsAlnum(Char) && Char != TEXT('_'))
		{
			Char = TEXT('_');
		}
	}
	return Out.IsEmpty() ? TEXT("StoryAsset") : Out;
}

UStoryEncounterPointDA* WritePoint(
	const FString& Arc,
	const FString& GraphAsset,
	const FName EncounterId,
	const TSharedPtr<FJsonObject>& PointObject,
	int32 PointIndex,
	TArray<UPackage*>& DirtyPackages,
	TArray<FString>& ReportLines)
{
	const FString AssetName = SanitizeAssetName(GetString(PointObject, TEXT("asset"), FString::Printf(TEXT("EP_%s_%d"), *GraphAsset, PointIndex + 1)));
	const FString PackagePath = FString::Printf(TEXT("%s/%s/%s/%s"), *PointRoot, *SanitizeAssetName(Arc), *SanitizeAssetName(GraphAsset), *AssetName);

	bool bCreated = false;
	UStoryEncounterPointDA* Point = LoadOrCreateAsset<UStoryEncounterPointDA>(PackagePath, DirtyPackages, bCreated);
	if (!Point)
	{
		ReportLines.Add(FString::Printf(TEXT("- Failed point `%s`."), *PackagePath));
		return nullptr;
	}

	Point->Modify();
	Point->EncounterId = EncounterId;
	Point->NodeId = FName(*GetString(PointObject, TEXT("nodeId"), FString::Printf(TEXT("point_%d"), PointIndex + 1)));
	Point->DisplayName = FText::FromString(GetString(PointObject, TEXT("displayName"), Point->NodeId.ToString()));
	Point->Kind = ParseNodeKind(GetString(PointObject, TEXT("kind")));
	Point->PlayerFacingEvent = FText::FromString(GetString(PointObject, TEXT("playerFacingEvent")));
	Point->FirePolicy = ParseFirePolicy(GetString(PointObject, TEXT("firePolicy")));
	Point->Condition = ParseCondition(GetObject(PointObject, TEXT("condition")));
	Point->PlacementLevel = FName(*GetString(PointObject, TEXT("placementLevel")));
	Point->PlacementName = FName(*GetString(PointObject, TEXT("placementName")));
	Point->EditorPosition = FVector2D(PointIndex * 360.f, 0.f);
	Point->Actions.Reset();
	for (const TSharedPtr<FJsonValue>& ActionValue : GetArray(PointObject, TEXT("actions")))
	{
		if (const TSharedPtr<FJsonObject>* ActionObject = nullptr; ActionValue->TryGetObject(ActionObject))
		{
			Point->Actions.Add(ParseAction(*ActionObject));
		}
	}

	Point->MarkPackageDirty();
	DirtyPackages.AddUnique(Point->GetPackage());
	ReportLines.Add(FString::Printf(TEXT("- %s point `%s`."), bCreated ? TEXT("Created") : TEXT("Updated"), *PackagePath));
	return Point;
}

void EnsureGraphEditorData(UStoryEncounterGraph* Graph)
{
	if (!Graph)
	{
		return;
	}
#if WITH_EDITORONLY_DATA
	if (!Graph->EdGraph)
	{
		Graph->EdGraph = CastChecked<UEdGraph_GenericGraph>(
			FBlueprintEditorUtils::CreateNewGraph(
				Graph,
				NAME_None,
				UEdGraph_GenericGraph::StaticClass(),
				UAssetGraphSchema_GenericGraph::StaticClass()));
		Graph->EdGraph->bAllowDeletion = false;
	}
#endif
}

UEdNode_GenericGraphNode* AddGraphNode(UStoryEncounterGraph* Graph, UStoryEncounterPointDA* Point, int32 NodeIndex)
{
	if (!Graph || !Point || !Graph->EdGraph)
	{
		return nullptr;
	}

	UEdGraph_GenericGraph* EdGraph = Cast<UEdGraph_GenericGraph>(Graph->EdGraph);
	if (!EdGraph)
	{
		return nullptr;
	}

	UEdNode_GenericGraphNode* EdNode = NewObject<UEdNode_GenericGraphNode>(EdGraph);
	UStoryEncounterGraphNode* GraphNode = NewObject<UStoryEncounterGraphNode>(EdNode, Graph->NodeType);
	GraphNode->Point = Point;
	GraphNode->FallbackEncounterId = Point->EncounterId;
	GraphNode->FallbackNodeId = Point->GetStableNodeId();
	GraphNode->FallbackTitle = Point->DisplayName;
	GraphNode->Graph = Graph;
	GraphNode->SetFlags(RF_Transactional);

	EdNode->GenericGraphNode = GraphNode;
	EdGraph->AddNode(EdNode, true, false);
	EdNode->CreateNewGuid();
	EdNode->PostPlacedNewNode();
	EdNode->AllocateDefaultPins();
	EdNode->NodePosX = NodeIndex * 360;
	EdNode->NodePosY = 0;
	EdNode->SetFlags(RF_Transactional);
	return EdNode;
}

void WriteGraph(
	const FString& Arc,
	const FString& GraphAsset,
	FName EncounterId,
	const FString& StoryId,
	const TArray<UStoryEncounterPointDA*>& Points,
	TArray<UPackage*>& DirtyPackages,
	TArray<FString>& ReportLines)
{
	const FString PackagePath = FString::Printf(TEXT("%s/%s/%s"), *GraphRoot, *SanitizeAssetName(Arc), *SanitizeAssetName(GraphAsset));
	bool bCreated = false;
	UStoryEncounterGraph* Graph = LoadOrCreateAsset<UStoryEncounterGraph>(PackagePath, DirtyPackages, bCreated);
	if (!Graph)
	{
		ReportLines.Add(FString::Printf(TEXT("- Failed graph `%s`."), *PackagePath));
		return;
	}

	Graph->Modify();
	Graph->EncounterId = EncounterId;
	Graph->DisplayName = FText::FromString(StoryId);
	Graph->Description = FText::FromString(FString::Printf(TEXT("Imported from StoryPipeline segment `%s`."), *StoryId));
	Graph->Name = GraphAsset;
	Graph->NodeType = UStoryEncounterGraphNode::StaticClass();
	Graph->EdgeType = UStoryEncounterGraphEdge::StaticClass();
	Graph->bEdgeEnabled = true;
#if WITH_EDITORONLY_DATA
	Graph->bCanRenameNode = true;
	Graph->bCanBeCyclical = false;
#endif

	EnsureGraphEditorData(Graph);
	UEdGraph_GenericGraph* EdGraph = Cast<UEdGraph_GenericGraph>(Graph->EdGraph);
	if (!EdGraph)
	{
		ReportLines.Add(FString::Printf(TEXT("- Graph `%s` has no editor graph."), *PackagePath));
		return;
	}

	EdGraph->Modify();
	EdGraph->Nodes.Reset();
	Graph->ClearGraph();

	TArray<UEdNode_GenericGraphNode*> EdNodes;
	for (int32 Index = 0; Index < Points.Num(); ++Index)
	{
		if (UEdNode_GenericGraphNode* EdNode = AddGraphNode(Graph, Points[Index], Index))
		{
			EdNodes.Add(EdNode);
		}
	}

	if (const UEdGraphSchema* Schema = EdGraph->GetSchema())
	{
		for (int32 Index = 0; Index + 1 < EdNodes.Num(); ++Index)
		{
			Schema->TryCreateConnection(EdNodes[Index]->Pins[1], EdNodes[Index + 1]->Pins[0]);
		}
	}

	EdGraph->RebuildGenericGraph();
	EdGraph->NotifyGraphChanged();
	Graph->MarkPackageDirty();
	DirtyPackages.AddUnique(Graph->GetPackage());
	ReportLines.Add(FString::Printf(TEXT("- %s graph `%s` with %d node(s)."),
		bCreated ? TEXT("Created") : TEXT("Updated"), *PackagePath, EdNodes.Num()));
}

FString ReadManifestPath(const FString& Params)
{
	FString PathValue;
	if (FParse::Value(*Params, TEXT("Manifest="), PathValue))
	{
		return FPaths::ConvertRelativePathToFull(PathValue);
	}
	return FPaths::ConvertRelativePathToFull(FPaths::ProjectDir(), DefaultManifestPath);
}
}

UStoryImportCommandlet::UStoryImportCommandlet()
{
	IsClient = false;
	IsEditor = true;
	IsServer = false;
	LogToConsole = true;
}

int32 UStoryImportCommandlet::Main(const FString& Params)
{
	using namespace StoryImport;

	const bool bApply = Params.Contains(TEXT("Apply"), ESearchCase::IgnoreCase);
	const FString ManifestPath = ReadManifestPath(Params);
	UE_LOG(LogTemp, Display, TEXT("[StoryImport] Mode=%s Manifest=%s"), bApply ? TEXT("Apply") : TEXT("DryRun"), *ManifestPath);

	FString JsonText;
	if (!FFileHelper::LoadFileToString(JsonText, *ManifestPath))
	{
		UE_LOG(LogTemp, Error, TEXT("[StoryImport] Failed to read manifest: %s"), *ManifestPath);
		return 1;
	}

	TSharedPtr<FJsonObject> RootObject;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonText);
	if (!FJsonSerializer::Deserialize(Reader, RootObject) || !RootObject.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("[StoryImport] Invalid JSON manifest."));
		return 1;
	}

	TArray<UPackage*> DirtyPackages;
	TArray<FString> ReportLines;
	ReportLines.Add(TEXT("# Story Import"));

	for (const TSharedPtr<FJsonValue>& SegmentValue : GetArray(RootObject, TEXT("segments")))
	{
		const TSharedPtr<FJsonObject>* SegmentObjectPtr = nullptr;
		if (!SegmentValue->TryGetObject(SegmentObjectPtr) || !SegmentObjectPtr || !SegmentObjectPtr->IsValid())
		{
			continue;
		}

		const TSharedPtr<FJsonObject>& SegmentObject = *SegmentObjectPtr;
		const FString StoryId = GetString(SegmentObject, TEXT("storyId"));
		const FString Arc = GetString(SegmentObject, TEXT("arc"), TEXT("main"));
		const FName EncounterId(*GetString(SegmentObject, TEXT("encounterId")));
		const FString GraphAsset = GetString(SegmentObject, TEXT("graphAsset"));
		ReportLines.Add(FString::Printf(TEXT("## %s"), *StoryId));

		if (StoryId.IsEmpty() || EncounterId.IsNone() || GraphAsset.IsEmpty())
		{
			ReportLines.Add(TEXT("- Skipped: storyId, encounterId, and graphAsset are required."));
			continue;
		}

		TArray<UStoryEncounterPointDA*> Points;
		int32 PointIndex = 0;
		for (const TSharedPtr<FJsonValue>& PointValue : GetArray(SegmentObject, TEXT("points")))
		{
			const TSharedPtr<FJsonObject>* PointObjectPtr = nullptr;
			if (!PointValue->TryGetObject(PointObjectPtr) || !PointObjectPtr || !PointObjectPtr->IsValid())
			{
				continue;
			}

			if (!bApply)
			{
				ReportLines.Add(FString::Printf(TEXT("- Would write point `%s`."), *GetString(*PointObjectPtr, TEXT("asset"))));
			}
			else if (UStoryEncounterPointDA* Point = WritePoint(Arc, GraphAsset, EncounterId, *PointObjectPtr, PointIndex, DirtyPackages, ReportLines))
			{
				Points.Add(Point);
			}
			++PointIndex;
		}

		if (!bApply)
		{
			ReportLines.Add(FString::Printf(TEXT("- Would write graph `%s` with %d point(s)."), *GraphAsset, PointIndex));
		}
		else
		{
			WriteGraph(Arc, GraphAsset, EncounterId, StoryId, Points, DirtyPackages, ReportLines);
		}
		ReportLines.Add(TEXT(""));
	}

	if (bApply && DirtyPackages.Num() > 0)
	{
		UEditorLoadingAndSavingUtils::SavePackages(DirtyPackages, false);
	}

	for (const FString& Line : ReportLines)
	{
		UE_LOG(LogTemp, Display, TEXT("%s"), *Line);
	}

	return 0;
}
