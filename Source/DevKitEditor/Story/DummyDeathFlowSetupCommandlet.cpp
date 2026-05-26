#include "Story/DummyDeathFlowSetupCommandlet.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "FileHelpers.h"
#include "FlowAsset.h"
#include "Graph/FlowGraph.h"
#include "Graph/FlowGraphSchema_Actions.h"
#include "Graph/Nodes/FlowGraphNode.h"
#include "GameModes/LevelFlowTypes.h"
#include "Map/RewardPickup.h"
#include "Misc/PackageName.h"
#include "Nodes/Graph/FlowNode_Start.h"
#include "Story/Flow/StoryFlowAsset.h"
#include "Story/Flow/Nodes/SNode_ShowHint.h"
#include "Story/Flow/Nodes/SNode_SpawnRewardPickup.h"
#include "Story/Encounter/StoryEncounterPointDataAsset.h"
#include "UObject/Package.h"

namespace DummyDeathFlowSetup
{
const FString FlowPackagePath = TEXT("/Game/Story/Flows/Tutorial/FA_DummyDeath_DropHeavyCard");
const FString PointObjectPath = TEXT("/Game/Story/EncounterPoints/Main_Tutorial_Demo/EG_FirstRun_Tutorial/EP_FirstRun_TrainingDummyCombo.EP_FirstRun_TrainingDummyCombo");
const FString PickupClassPath = TEXT("/Game/Code/Dungeon/BP_RewardPickup.BP_RewardPickup_C");
const FString RuneObjectPath = TEXT("/Game/Docs/BuffDocs/V2-RuneCard/512Generated/DA_Rune512_Heavy.DA_Rune512_Heavy");

FString ToObjectPath(const FString& PackagePath)
{
	return PackagePath + TEXT(".") + FPackageName::GetLongPackageAssetName(PackagePath);
}

UStoryFlowAsset* LoadOrCreateFlowAsset(TArray<UPackage*>& DirtyPackages, bool& bOutCreated)
{
	bOutCreated = false;

	if (UStoryFlowAsset* Existing = LoadObject<UStoryFlowAsset>(nullptr, *ToObjectPath(FlowPackagePath)))
	{
		return Existing;
	}

	// 若路径已存在其他类型的 FA（例如旧的 ULevelFlowAsset），提示用户手动删除
	if (UFlowAsset* OldAsset = LoadObject<UFlowAsset>(nullptr, *ToObjectPath(FlowPackagePath)))
	{
		UE_LOG(LogTemp, Error,
			TEXT("[DummyDeathFlowSetup] %s already exists as %s (not UStoryFlowAsset). Delete it in the content browser and re-run."),
			*FlowPackagePath, *OldAsset->GetClass()->GetName());
		return nullptr;
	}

	UPackage* Package = CreatePackage(*FlowPackagePath);
	if (!Package)
	{
		return nullptr;
	}

	UStoryFlowAsset* FlowAsset = NewObject<UStoryFlowAsset>(
		Package,
		*FPackageName::GetLongPackageAssetName(FlowPackagePath),
		RF_Public | RF_Standalone | RF_Transactional);
	if (!FlowAsset)
	{
		return nullptr;
	}

	UFlowGraph::CreateGraph(FlowAsset);
	FAssetRegistryModule::AssetCreated(FlowAsset);
	FlowAsset->MarkPackageDirty();
	DirtyPackages.AddUnique(Package);
	bOutCreated = true;
	return FlowAsset;
}

USNode_SpawnRewardPickup* FindSpawnRewardNode(UStoryFlowAsset* FlowAsset)
{
	if (!FlowAsset)
	{
		return nullptr;
	}

	for (const TPair<FGuid, UFlowNode*>& Pair : FlowAsset->GetNodes())
	{
		if (USNode_SpawnRewardPickup* SpawnNode = Cast<USNode_SpawnRewardPickup>(Pair.Value))
		{
			return SpawnNode;
		}
	}

	return nullptr;
}

USNode_ShowHint* FindPickupHintNode(UStoryFlowAsset* FlowAsset)
{
	if (!FlowAsset)
	{
		return nullptr;
	}

	for (const TPair<FGuid, UFlowNode*>& Pair : FlowAsset->GetNodes())
	{
		if (USNode_ShowHint* HintNode = Cast<USNode_ShowHint>(Pair.Value))
		{
			if (HintNode->HintText.ToString().Contains(TEXT("拾取"))
				|| HintNode->HintText.ToString().Contains(TEXT("pick"), ESearchCase::IgnoreCase))
			{
				return HintNode;
			}
		}
	}

	return nullptr;
}

UFlowNode_Start* FindStartNode(UStoryFlowAsset* FlowAsset)
{
	if (!FlowAsset)
	{
		return nullptr;
	}

	for (const TPair<FGuid, UFlowNode*>& Pair : FlowAsset->GetNodes())
	{
		if (UFlowNode_Start* StartNode = Cast<UFlowNode_Start>(Pair.Value))
		{
			return StartNode;
		}
	}

	return nullptr;
}

bool EnsurePinConnection(UEdGraph* Graph, UEdGraphPin* FromPin, UEdGraphPin* ToPin)
{
	if (!Graph || !FromPin || !ToPin)
	{
		return false;
	}

	if (FromPin->LinkedTo.Contains(ToPin))
	{
		return true;
	}

	FromPin->Modify();
	ToPin->Modify();
	if (const UEdGraphSchema* Schema = Graph->GetSchema())
	{
		return Schema->TryCreateConnection(FromPin, ToPin);
	}
	return false;
}

bool ConfigureRewardNode(USNode_SpawnRewardPickup* SpawnNode)
{
	if (!SpawnNode)
	{
		return false;
	}

	TSubclassOf<ARewardPickup> PickupClass = LoadClass<ARewardPickup>(nullptr, *PickupClassPath);
	if (!PickupClass)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to load reward pickup class %s."), *PickupClassPath);
		return false;
	}

	URuneDataAsset* RuneAsset = LoadObject<URuneDataAsset>(nullptr, *RuneObjectPath);
	if (!RuneAsset)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to load rune asset %s."), *RuneObjectPath);
		return false;
	}

	FLootOption LootOption;
	LootOption.LootType = ELootType::Rune;
	LootOption.RuneAsset = RuneAsset;

	SpawnNode->Modify();
	SpawnNode->RewardPickupClass = PickupClass;
	SpawnNode->RewardLootOptions.Reset();
	SpawnNode->RewardLootOptions.Add(LootOption);
	SpawnNode->RewardPickupCount = 1;
	SpawnNode->RewardSpawnOffset = FVector(120.f, 0.f, 20.f);
	SpawnNode->bAllowPickupOutsideArrangement = true;
	return true;
}

USNode_SpawnRewardPickup* EnsureSpawnRewardNode(UStoryFlowAsset* FlowAsset, TArray<UPackage*>& DirtyPackages)
{
	if (!FlowAsset)
	{
		return nullptr;
	}

	UEdGraph* Graph = FlowAsset->GetGraph();
	if (!Graph)
	{
		UFlowGraph::CreateGraph(FlowAsset);
		Graph = FlowAsset->GetGraph();
	}

	UFlowNode_Start* StartNode = FindStartNode(FlowAsset);
	UFlowGraphNode* StartGraphNode = StartNode ? Cast<UFlowGraphNode>(StartNode->GetGraphNode()) : nullptr;
	if (!Graph || !StartGraphNode)
	{
		UE_LOG(LogTemp, Error, TEXT("Flow asset %s has no usable start node."), *FlowPackagePath);
		return nullptr;
	}

	USNode_SpawnRewardPickup* SpawnNode = FindSpawnRewardNode(FlowAsset);
	UFlowGraphNode* SpawnGraphNode = SpawnNode ? Cast<UFlowGraphNode>(SpawnNode->GetGraphNode()) : nullptr;
	if (!SpawnNode || !SpawnGraphNode)
	{
		SpawnGraphNode = FFlowGraphSchemaAction_NewNode::CreateNode(
			Graph,
			StartGraphNode->GetOutputPin(0),
			USNode_SpawnRewardPickup::StaticClass(),
			FVector2D(320.f, 0.f),
			false);
		SpawnNode = SpawnGraphNode ? Cast<USNode_SpawnRewardPickup>(SpawnGraphNode->GetFlowNodeBase()) : nullptr;
	}
	else if (UEdGraphPin* StartOut = StartGraphNode->GetOutputPin(0))
	{
		UEdGraphPin* SpawnIn = SpawnGraphNode->GetInputPin(0);
		if (SpawnIn && !StartOut->LinkedTo.Contains(SpawnIn))
		{
			StartOut->Modify();
			SpawnIn->Modify();
			if (const UEdGraphSchema* Schema = Graph->GetSchema())
			{
				Schema->TryCreateConnection(StartOut, SpawnIn);
			}
		}
	}

	if (!ConfigureRewardNode(SpawnNode))
	{
		return nullptr;
	}

	USNode_ShowHint* HintNode = FindPickupHintNode(FlowAsset);
	UFlowGraphNode* HintGraphNode = HintNode ? Cast<UFlowGraphNode>(HintNode->GetGraphNode()) : nullptr;
	if (!HintNode || !HintGraphNode)
	{
		HintGraphNode = FFlowGraphSchemaAction_NewNode::CreateNode(
			Graph,
			SpawnGraphNode ? SpawnGraphNode->GetOutputPin(0) : nullptr,
			USNode_ShowHint::StaticClass(),
			FVector2D(640.f, 0.f),
			false);
		HintNode = HintGraphNode ? Cast<USNode_ShowHint>(HintGraphNode->GetFlowNodeBase()) : nullptr;
	}
	else if (SpawnGraphNode)
	{
		EnsurePinConnection(Graph, SpawnGraphNode->GetOutputPin(0), HintGraphNode->GetInputPin(0));
	}

	if (HintNode)
	{
		HintNode->Modify();
		HintNode->HintTitle = FText::GetEmpty();
		HintNode->HintText = FText::FromString(TEXT("掉落了一张新卡牌。靠近掉落物，按 <input action=\"Interact\"/> 拾取。"));
		HintNode->Duration = 3.0f;
	}

	FlowAsset->HarvestNodeConnections();
	FlowAsset->MarkPackageDirty();
	DirtyPackages.AddUnique(FlowAsset->GetPackage());
	return SpawnNode;
}

int32 RemoveInlineActionsHandledByFlow(UStoryEncounterPointDA* Point)
{
	if (!Point)
	{
		return 0;
	}

	const int32 OldNum = Point->Actions.Num();
	Point->Actions.RemoveAll([](const FStoryEncounterAction& Action)
	{
		return Action.Kind == EStoryEncounterActionKind::SpawnRewardPickup
			|| Action.Kind == EStoryEncounterActionKind::WeakHint
			|| Action.Kind == EStoryEncounterActionKind::TutorialAreaHint
			|| Action.Kind == EStoryEncounterActionKind::TutorialPopup;
	});
	return OldNum - Point->Actions.Num();
}
}

UDummyDeathFlowSetupCommandlet::UDummyDeathFlowSetupCommandlet()
{
	IsClient = false;
	IsEditor = true;
	IsServer = false;
	LogToConsole = true;
}

int32 UDummyDeathFlowSetupCommandlet::Main(const FString& Params)
{
	using namespace DummyDeathFlowSetup;

	TArray<UPackage*> DirtyPackages;
	bool bCreatedFlow = false;
	UStoryFlowAsset* FlowAsset = LoadOrCreateFlowAsset(DirtyPackages, bCreatedFlow);
	if (!FlowAsset)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create/load %s."), *FlowPackagePath);
		return 1;
	}

	USNode_SpawnRewardPickup* SpawnNode = EnsureSpawnRewardNode(FlowAsset, DirtyPackages);
	if (!SpawnNode)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to configure Spawn Reward Pickup node."));
		return 1;
	}

	UStoryEncounterPointDA* Point = LoadObject<UStoryEncounterPointDA>(nullptr, *PointObjectPath);
	if (!Point)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to load story point %s."), *PointObjectPath);
		return 1;
	}

	Point->Modify();
	Point->Kind = EStoryEncounterNodeKind::Death;
	Point->Condition.Kind = EStoryEncounterConditionKind::None;
	Point->FirePolicy = EStoryEncounterFirePolicy::Once;
	const int32 RemovedInlineActions = RemoveInlineActionsHandledByFlow(Point);
	Point->NodeEventFlow = FlowAsset;
	Point->MarkPackageDirty();
	DirtyPackages.AddUnique(Point->GetPackage());

	const bool bSaved = UEditorLoadingAndSavingUtils::SavePackages(DirtyPackages, false);
	if (!bSaved)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to save one or more packages."));
		return 1;
	}

	UE_LOG(LogTemp, Display, TEXT("[DummyDeathFlowSetup] %s %s, configured spawn node %s, bound NodeEventFlow on %s, removed %d inline action(s)."),
		bCreatedFlow ? TEXT("Created") : TEXT("Updated"),
		*FlowPackagePath,
		*GetNameSafe(SpawnNode),
		*PointObjectPath,
		RemovedInlineActions);
	return 0;
}
