#include "Story/StoryEncounterTutorialMigrationCommandlet.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "FileHelpers.h"
#include "FlowAsset.h"
#include "Graph/FlowGraph.h"
#include "Graph/FlowGraphSchema_Actions.h"
#include "Graph/Nodes/FlowGraphNode.h"
#include "GenericGraphAssetEditor/AssetGraphSchema_GenericGraph.h"
#include "GenericGraphAssetEditor/EdGraph_GenericGraph.h"
#include "GenericGraphAssetEditor/EdNode_GenericGraphNode.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Misc/PackageName.h"
#include "Nodes/Graph/FlowNode_Start.h"
#include "Story/Encounter/StoryEncounterGraph.h"
#include "Story/Encounter/StoryEncounterGraphEdge.h"
#include "Story/Encounter/StoryEncounterGraphNode.h"
#include "Story/Encounter/StoryEncounterPointDataAsset.h"
#include "Story/Flow/StoryFlowAsset.h"
#include "Story/Flow/Nodes/SNode_RecordProgress.h"
#include "Story/Flow/Nodes/SNode_ShowHint.h"
#include "Story/Flow/Nodes/SNode_ShowTutorialPopup.h"
#include "Story/Flow/Nodes/SNode_TutorialAreaHint.h"
#include "UObject/Package.h"

namespace StoryEncounterTutorialMigration
{
struct FActionSpec
{
	EStoryEncounterActionKind Kind = EStoryEncounterActionKind::WeakHint;
	FString Title;
	FString Body;
	FName TutorialEventId;
	FName ProgressKey;
	FString ProgressLabel;
	bool bPauseGame = true;
};

struct FNodeSpec
{
	FName NodeId;
	FString Title;
	EStoryEncounterNodeKind Kind = EStoryEncounterNodeKind::System;
	FString PlayerFacingEvent;
	TArray<FActionSpec> Actions;
};

struct FFlowSpec
{
	FName EncounterId;
	FString AssetName;
	FString DisplayName;
	FString Description;
	FString PointFolder;
	TArray<FNodeSpec> Nodes;
};

const FString GraphRoot = TEXT("/Game/Story/Encounters/Tutorial");
const FString PointRoot = TEXT("/Game/Story/EncounterPoints/Tutorial");
const FString FlowRoot = TEXT("/Game/Story/Flows/Tutorial/Generated");

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

FStoryEncounterAction MakeAction(const FActionSpec& Spec)
{
	FStoryEncounterAction Action;
	Action.Kind = Spec.Kind;
	Action.Title = FText::FromString(Spec.Title);
	Action.Body = FText::FromString(Spec.Body);
	Action.TutorialEventId = Spec.TutorialEventId;
	Action.bPauseGame = Spec.bPauseGame;
	Action.ProgressKey = Spec.ProgressKey;
	Action.ProgressLabel = FText::FromString(Spec.ProgressLabel);
	return Action;
}

FActionSpec WeakHint(const FString& Title, const FString& Body)
{
	FActionSpec Action;
	Action.Kind = EStoryEncounterActionKind::WeakHint;
	Action.Title = Title;
	Action.Body = Body;
	return Action;
}

FActionSpec Dialogue(const FString& Title, const FString& Body)
{
	FActionSpec Action;
	Action.Kind = EStoryEncounterActionKind::Dialogue;
	Action.Title = Title;
	Action.Body = Body;
	return Action;
}

FActionSpec TutorialPopup(FName EventId, bool bPauseGame = true)
{
	FActionSpec Action;
	Action.Kind = EStoryEncounterActionKind::TutorialPopup;
	Action.TutorialEventId = EventId;
	Action.bPauseGame = bPauseGame;
	return Action;
}

FActionSpec RecordProgress(FName ProgressKey, const FString& Label)
{
	FActionSpec Action;
	Action.Kind = EStoryEncounterActionKind::RecordProgress;
	Action.ProgressKey = ProgressKey;
	Action.ProgressLabel = Label;
	return Action;
}

UEdGraphPin* FindGraphPin(UFlowNode* Node, FName PinName, EEdGraphPinDirection Direction)
{
	UFlowGraphNode* GraphNode = Node ? Cast<UFlowGraphNode>(Node->GetGraphNode()) : nullptr;
	if (!GraphNode)
	{
		return nullptr;
	}

	for (UEdGraphPin* Pin : GraphNode->Pins)
	{
		if (Pin && Pin->Direction == Direction && Pin->PinName == PinName)
		{
			return Pin;
		}
	}
	return nullptr;
}

void ClearNonEntryNodes(UFlowAsset* FlowAsset)
{
	UFlowGraph* FlowGraph = FlowAsset ? Cast<UFlowGraph>(FlowAsset->GetGraph()) : nullptr;
	UFlowNode* EntryNode = FlowAsset ? FlowAsset->GetDefaultEntryNode() : nullptr;
	if (!FlowAsset || !FlowGraph || !EntryNode)
	{
		return;
	}

	TArray<TPair<FGuid, UFlowNode*>> NodesToRemove;
	for (const TPair<FGuid, UFlowNode*>& Pair : FlowAsset->GetNodes())
	{
		if (Pair.Value && Pair.Value != EntryNode)
		{
			NodesToRemove.Add(Pair);
		}
	}

	for (const TPair<FGuid, UFlowNode*>& Pair : NodesToRemove)
	{
		if (UFlowGraphNode* GraphNode = Cast<UFlowGraphNode>(Pair.Value->GetGraphNode()))
		{
			FlowGraph->GetSchema()->BreakNodeLinks(*GraphNode);
			GraphNode->DestroyNode();
		}
		FlowAsset->UnregisterNode(Pair.Key);
	}
}

UFlowNode* CreateStoryFlowNode(UFlowGraph* FlowGraph, UFlowNode* FromNode, UClass* NodeClass, const FVector2D& Location)
{
	UEdGraphPin* FromPin = FromNode ? FindGraphPin(FromNode, TEXT("Out"), EGPD_Output) : nullptr;
	UFlowGraphNode* GraphNode = FFlowGraphSchemaAction_NewNode::CreateNode(FlowGraph, FromPin, NodeClass, Location, false);
	return GraphNode ? Cast<UFlowNode>(GraphNode->GetFlowNodeBase()) : nullptr;
}

UFlowNode* AppendActionNode(UFlowGraph* FlowGraph, UFlowNode* PreviousNode,
	const FFlowSpec& Flow, const FActionSpec& Spec, int32 ActionIndex)
{
	const FVector2D Location(320.f + ActionIndex * 320.f, 0.f);
	switch (Spec.Kind)
	{
	case EStoryEncounterActionKind::Dialogue:
	case EStoryEncounterActionKind::WeakHint:
		if (USNode_ShowHint* HintNode = Cast<USNode_ShowHint>(
			CreateStoryFlowNode(FlowGraph, PreviousNode, USNode_ShowHint::StaticClass(), Location)))
		{
			HintNode->HintTitle = Spec.Kind == EStoryEncounterActionKind::Dialogue
				? FText::FromString(Spec.Title)
				: FText::GetEmpty();
			HintNode->HintText = FText::FromString(Spec.Body);
			HintNode->Duration = 3.0f;
			return HintNode;
		}
		break;

	case EStoryEncounterActionKind::TutorialAreaHint:
		if (USNode_TutorialAreaHint* AreaHintNode = Cast<USNode_TutorialAreaHint>(
			CreateStoryFlowNode(FlowGraph, PreviousNode, USNode_TutorialAreaHint::StaticClass(), Location)))
		{
			AreaHintNode->HintText = FText::FromString(Spec.Body);
			AreaHintNode->Duration = 0.0f;
			return AreaHintNode;
		}
		break;

	case EStoryEncounterActionKind::TutorialPopup:
		if (USNode_ShowTutorialPopup* PopupNode = Cast<USNode_ShowTutorialPopup>(
			CreateStoryFlowNode(FlowGraph, PreviousNode, USNode_ShowTutorialPopup::StaticClass(), Location)))
		{
			PopupNode->TutorialEventId = Spec.TutorialEventId;
			PopupNode->bPauseGame = Spec.bPauseGame;
			return PopupNode;
		}
		break;

	case EStoryEncounterActionKind::RecordProgress:
		if (USNode_RecordProgress* ProgressNode = Cast<USNode_RecordProgress>(
			CreateStoryFlowNode(FlowGraph, PreviousNode, USNode_RecordProgress::StaticClass(), Location)))
		{
			ProgressNode->EncounterId = Flow.EncounterId;
			ProgressNode->ProgressKey = Spec.ProgressKey;
			return ProgressNode;
		}
		break;

	default:
		break;
	}

	return PreviousNode;
}

bool IsHandledByStoryFlow(const FActionSpec& Spec)
{
	switch (Spec.Kind)
	{
	case EStoryEncounterActionKind::Dialogue:
	case EStoryEncounterActionKind::WeakHint:
	case EStoryEncounterActionKind::TutorialAreaHint:
	case EStoryEncounterActionKind::TutorialPopup:
	case EStoryEncounterActionKind::RecordProgress:
		return true;
	default:
		return false;
	}
}

TArray<FFlowSpec> MakeTutorialFlows()
{
	return {
		{
			TEXT("EM_MemoryTutorial_PreRun"),
			TEXT("EG_MemoryTutorial_PreRun"),
			TEXT("第一局前：记忆碎片基础教学"),
			TEXT("玩家作为教宗的灵魂碎片体验基础操作。这里弱引导移动、攻击、闪避和卡牌结算预告，最后以骑士团成员剧情杀切到正式第一局。"),
			TEXT("MemoryPreRun"),
			{
				{
					TEXT("memory_awaken"),
					TEXT("苏醒"),
					EStoryEncounterNodeKind::System,
					TEXT("玩家在记忆碎片里醒来，只能依靠直觉控制身体。"),
					{ WeakHint(TEXT("醒来"), TEXT("先让身体动起来。")),
						RecordProgress(TEXT("memory_awaken_seen"), TEXT("已体验记忆苏醒")) }
				},
				{
					TEXT("move_camera"),
					TEXT("移动与观察"),
					EStoryEncounterNodeKind::Area,
					TEXT("玩家在安全空间里移动、转向、观察出口和残影。"),
					{ WeakHint(TEXT("移动与观察"), TEXT("沿着光和残影前进。")),
						RecordProgress(TEXT("move_camera_done"), TEXT("已完成移动观察")) }
				},
				{
					TEXT("basic_attack"),
					TEXT("攻击与战技"),
					EStoryEncounterNodeKind::Feature,
					TEXT("玩家面对低威胁残影，尝试普通攻击和武器战技的节奏差异。"),
					{ WeakHint(TEXT("攻击残影"), TEXT("攻击负责稳定推进卡组，战技负责引爆终结。")),
						RecordProgress(TEXT("basic_attack_done"), TEXT("已体验基础攻击")) }
				},
				{
					TEXT("dodge_spacing"),
					TEXT("闪避与距离"),
					EStoryEncounterNodeKind::Feature,
					TEXT("玩家看到敌人攻击前摇红光，学习拉开距离或闪避。"),
					{ WeakHint(TEXT("红光"), TEXT("敌人动作发红时，先躲开。")),
						RecordProgress(TEXT("dodge_done"), TEXT("已体验闪避")) }
				},
				{
					TEXT("card_consumption_hint"),
					TEXT("卡牌结算预告"),
					EStoryEncounterNodeKind::Feature,
					TEXT("攻击时下方卡牌顺序发生变化，但不在记忆碎片里完整解释符文系统。"),
					{ WeakHint(TEXT("卡牌在燃尽"), TEXT("攻击会推动身上的符文卡顺序。现在只需要观察它变化。")),
						RecordProgress(TEXT("card_consumption_seen"), TEXT("已看到攻击推进卡牌")) }
				},
				{
					TEXT("boss_execution"),
					TEXT("骑士团剧情杀"),
					EStoryEncounterNodeKind::Death,
					TEXT("骑士团成员出现并击碎记忆，玩家允许失败后回到正式开始状态。"),
					{ Dialogue(TEXT("记忆断裂"), TEXT("这不是你能赢下的战斗。记忆在刀光里断开。")),
						RecordProgress(TEXT("boss_execution_seen"), TEXT("已触发剧情杀")) }
				},
				{
					TEXT("memory_complete"),
					TEXT("进入正式第一局"),
					EStoryEncounterNodeKind::System,
					TEXT("记忆碎片结束，玩家进入主城或正式第一局流程。"),
					{ WeakHint(TEXT("残魂稳定"), TEXT("香炉重新收束了你的灵魂。")),
						RecordProgress(TEXT("memory_fragment_complete"), TEXT("记忆碎片教学完成")) }
				},
			}
		},
		{
			TEXT("EM_FirstRun_Tutorial"),
			TEXT("EG_FirstRun_Tutorial"),
			TEXT("第一局：正式玩法教学"),
			TEXT("正式第一局按弱引导节奏解释武器、攻击结算卡牌、洗牌、首枚符文、背包整理、连携和路线选择。"),
			TEXT("FirstRun"),
			{
				{
					TEXT("hub_start"),
					TEXT("主城醒来"),
					EStoryEncounterNodeKind::System,
					TEXT("玩家在主城稳定残魂，《遗圣目录》给出冷冰冰的第一条牵引。"),
					{ Dialogue(TEXT("遗圣目录"), TEXT("候圣体稳定。继续前进。")),
						RecordProgress(TEXT("hub_start_seen"), TEXT("已进入首局主城引导")) }
				},
				{
					TEXT("weapon_pickup"),
					TEXT("武器与初始卡组"),
					EStoryEncounterNodeKind::Object,
					TEXT("玩家拾取武器，看见 1D 卡组条。"),
					{ TutorialPopup(TEXT("tutorial_weapon_pickup")),
						RecordProgress(TEXT("weapon_pickup_seen"), TEXT("已看过武器与卡组教程")) }
				},
				{
					TEXT("attack_card_consume"),
					TEXT("攻击结算当前卡"),
					EStoryEncounterNodeKind::Feature,
					TEXT("玩家正式理解攻击会按顺序结算卡牌能力。"),
					{ WeakHint(TEXT("当前卡"), TEXT("每次攻击会尝试结算当前卡牌能力，然后推进到下一张。")),
						RecordProgress(TEXT("attack_card_consume_seen"), TEXT("已理解攻击结算当前卡")) }
				},
				{
					TEXT("shuffle_observe"),
					TEXT("洗牌回填"),
					EStoryEncounterNodeKind::Feature,
					TEXT("玩家打空卡组并观察洗牌回填。"),
					{ TutorialPopup(TEXT("tutorial_shuffle_hint"), false),
						RecordProgress(TEXT("shuffle_seen"), TEXT("已看过洗牌提示")) }
				},
				{
					TEXT("first_rune_reward"),
					TEXT("获得第一枚符文"),
					EStoryEncounterNodeKind::Object,
					TEXT("玩家清场后获得第一枚新符文/卡牌，理解战斗中会有收获。"),
					{ TutorialPopup(TEXT("tutorial_first_rune")),
						RecordProgress(TEXT("first_rune_seen"), TEXT("已获得第一枚符文")) }
				},
				{
					TEXT("backpack_arrange"),
					TEXT("背包整理"),
					EStoryEncounterNodeKind::Feature,
					TEXT("玩家打开背包，调整卡牌顺序或理解背包是构筑入口。"),
					{ TutorialPopup(TEXT("tutorial_backpack")),
						RecordProgress(TEXT("backpack_seen"), TEXT("已看过背包整理教程")) }
				},
				{
					TEXT("card_link_first"),
					TEXT("第一次连携"),
					EStoryEncounterNodeKind::Feature,
					TEXT("玩家获得或触发第一张连携卡，体验探索、尝试、发现的构筑感。"),
					{ TutorialPopup(TEXT("tutorial_card_link")),
						RecordProgress(TEXT("card_link_seen"), TEXT("已看过连携教程")) }
				},
				{
					TEXT("route_choice"),
					TEXT("路线选择"),
					EStoryEncounterNodeKind::Area,
					TEXT("玩家第一次面对本局变强与局外投资的路线取舍。"),
					{ WeakHint(TEXT("两条路"), TEXT("一条路让本局更强，一条路把资源带回更远的地方。")),
						RecordProgress(TEXT("route_choice_seen"), TEXT("已看过路线选择提示")) }
				},
				{
					TEXT("first_run_complete"),
					TEXT("首局教学完成"),
					EStoryEncounterNodeKind::System,
					TEXT("首局核心教学闭环完成，后续局外成长可以在第 3-5 次开局后展开。"),
					{ WeakHint(TEXT("目录合拢"), TEXT("你已经知道如何从战斗里带走东西。")),
						RecordProgress(TEXT("first_run_tutorial_complete"), TEXT("首局正式教学完成")) }
				},
			}
		},
	};
}

UStoryFlowAsset* WriteStoryFlowAsset(const FFlowSpec& Flow, const FNodeSpec& Node,
	TArray<UPackage*>& DirtyPackages, TArray<FString>& ReportLines)
{
	const bool bHasHandledActions = Node.Actions.ContainsByPredicate([](const FActionSpec& Action)
	{
		return IsHandledByStoryFlow(Action);
	});
	if (!bHasHandledActions)
	{
		return nullptr;
	}

	const FString PackagePath = FString::Printf(TEXT("%s/%s/FA_%s_%s"),
		*FlowRoot,
		*Flow.PointFolder,
		*Flow.AssetName,
		*Node.NodeId.ToString());

	bool bCreated = false;
	UStoryFlowAsset* FlowAsset = LoadOrCreateAsset<UStoryFlowAsset>(PackagePath, DirtyPackages, bCreated);
	if (!FlowAsset)
	{
		ReportLines.Add(FString::Printf(TEXT("- Failed Story FA `%s`."), *PackagePath));
		return nullptr;
	}

	FlowAsset->Modify();
	if (!FlowAsset->GetGraph())
	{
		UFlowGraph::CreateGraph(FlowAsset);
	}

	UFlowGraph* FlowGraph = Cast<UFlowGraph>(FlowAsset->GetGraph());
	UFlowNode* StartNode = FlowAsset->GetDefaultEntryNode();
	if (!FlowGraph || !StartNode)
	{
		ReportLines.Add(FString::Printf(TEXT("- Story FA `%s` has no usable Start node."), *PackagePath));
		return nullptr;
	}

	ClearNonEntryNodes(FlowAsset);

	UFlowNode* PreviousNode = StartNode;
	int32 ActionIndex = 0;
	int32 WrittenNodeCount = 0;
	for (const FActionSpec& ActionSpec : Node.Actions)
	{
		if (!IsHandledByStoryFlow(ActionSpec))
		{
			continue;
		}

		UFlowNode* NewPreviousNode = AppendActionNode(FlowGraph, PreviousNode, Flow, ActionSpec, ActionIndex);
		if (NewPreviousNode && NewPreviousNode != PreviousNode)
		{
			PreviousNode = NewPreviousNode;
			++WrittenNodeCount;
			++ActionIndex;
		}
	}

	FlowAsset->HarvestNodeConnections();
	FlowAsset->PostEditChange();
	FlowAsset->MarkPackageDirty();
	DirtyPackages.AddUnique(FlowAsset->GetPackage());
	FlowGraph->NotifyGraphChanged();

	ReportLines.Add(FString::Printf(TEXT("- %s Story FA `%s` with %d node(s)."),
		bCreated ? TEXT("Created") : TEXT("Updated"),
		*PackagePath,
		WrittenNodeCount));
	return FlowAsset;
}

UStoryEncounterPointDA* WritePointAsset(const FFlowSpec& Flow, const FNodeSpec& Node, int32 NodeIndex,
	TArray<UPackage*>& DirtyPackages, TArray<FString>& ReportLines)
{
	const FString PackagePath = FString::Printf(TEXT("%s/%s/EP_%s_%s"),
		*PointRoot,
		*Flow.PointFolder,
		*Flow.AssetName,
		*Node.NodeId.ToString());

	bool bCreated = false;
	UStoryEncounterPointDA* Point = LoadOrCreateAsset<UStoryEncounterPointDA>(PackagePath, DirtyPackages, bCreated);
	if (!Point)
	{
		ReportLines.Add(FString::Printf(TEXT("- Failed point `%s`."), *PackagePath));
		return nullptr;
	}

	Point->Modify();
	Point->EncounterId = Flow.EncounterId;
	Point->NodeId = Node.NodeId;
	Point->DisplayName = FText::FromString(Node.Title);
	Point->Kind = Node.Kind;
	Point->PlayerFacingEvent = FText::FromString(Node.PlayerFacingEvent);
	Point->FirePolicy = EStoryEncounterFirePolicy::Once;
	Point->Actions.Reset();
	Point->PlacementLevel = NAME_None;
	Point->PlacementName = Node.NodeId;
	Point->EditorPosition = FVector2D(NodeIndex * 360.f, 0.f);
	Point->NodeEventFlow = WriteStoryFlowAsset(Flow, Node, DirtyPackages, ReportLines);

	for (const FActionSpec& ActionSpec : Node.Actions)
	{
		if (!IsHandledByStoryFlow(ActionSpec))
		{
			Point->Actions.Add(MakeAction(ActionSpec));
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

void WriteGraphAsset(const FFlowSpec& Flow, const TArray<UStoryEncounterPointDA*>& Points,
	TArray<UPackage*>& DirtyPackages, TArray<FString>& ReportLines)
{
	const FString PackagePath = FString::Printf(TEXT("%s/%s"), *GraphRoot, *Flow.AssetName);
	bool bCreated = false;
	UStoryEncounterGraph* Graph = LoadOrCreateAsset<UStoryEncounterGraph>(PackagePath, DirtyPackages, bCreated);
	if (!Graph)
	{
		ReportLines.Add(FString::Printf(TEXT("- Failed graph `%s`."), *PackagePath));
		return;
	}

	Graph->Modify();
	Graph->EncounterId = Flow.EncounterId;
	Graph->DisplayName = FText::FromString(Flow.DisplayName);
	Graph->Description = FText::FromString(Flow.Description);
	Graph->Name = Flow.AssetName;
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
		bCreated ? TEXT("Created") : TEXT("Updated"),
		*PackagePath,
		EdNodes.Num()));
}
}

UStoryEncounterTutorialMigrationCommandlet::UStoryEncounterTutorialMigrationCommandlet()
{
	IsClient = false;
	IsEditor = true;
	IsServer = false;
	LogToConsole = true;
}

int32 UStoryEncounterTutorialMigrationCommandlet::Main(const FString& Params)
{
	using namespace StoryEncounterTutorialMigration;

	const bool bApply = Params.Contains(TEXT("Apply"), ESearchCase::IgnoreCase);
	if (!bApply)
	{
		UE_LOG(LogTemp, Display, TEXT("Story encounter tutorial migration is dry-run. Pass Apply to write assets."));
	}

	TArray<UPackage*> DirtyPackages;
	TArray<FString> ReportLines;
	ReportLines.Add(TEXT("# Story Encounter Tutorial Migration"));

	for (const FFlowSpec& Flow : MakeTutorialFlows())
	{
		ReportLines.Add(FString::Printf(TEXT("## %s"), *Flow.DisplayName));
		TArray<UStoryEncounterPointDA*> Points;
		for (int32 Index = 0; Index < Flow.Nodes.Num(); ++Index)
		{
			if (!bApply)
			{
				ReportLines.Add(FString::Printf(TEXT("- Would write point `%s`."), *Flow.Nodes[Index].NodeId.ToString()));
				continue;
			}
			Points.Add(WritePointAsset(Flow, Flow.Nodes[Index], Index, DirtyPackages, ReportLines));
		}

		if (bApply)
		{
			WriteGraphAsset(Flow, Points, DirtyPackages, ReportLines);
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
