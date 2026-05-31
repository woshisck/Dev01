#include "Story/FirstRunTutorialSpawnerSetupCommandlet.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Character/EnemyCharacterBase.h"
#include "FileHelpers.h"
#include "FlowAsset.h"
#include "Graph/FlowGraph.h"
#include "Graph/FlowGraphSchema_Actions.h"
#include "Graph/Nodes/FlowGraphNode.h"
#include "Misc/PackageName.h"
#include "Nodes/Graph/FlowNode_Start.h"
#include "Story/Flow/StoryFlowAsset.h"
#include "Story/Flow/Nodes/SNode_ActivateTutorialSpawner.h"
#include "Story/Flow/Nodes/SNode_SetActorEnabled.h"
#include "Story/Encounter/StoryEncounterPointDataAsset.h"
#include "UObject/Package.h"

namespace FirstRunTutorialSpawnerSetup
{
const FString ActivateFlowPath = TEXT("/Game/Story/Flows/Tutorial/FA_ActivateTutorialDummySpawner");
const FString HubMovePointPath = TEXT("/Game/Story/EncounterPoints/Main_Tutorial_Demo/EG_FirstRun_Tutorial/EP_FirstRun_HubMoveHint");
const FString PickupPointPath = TEXT("/Game/Story/EncounterPoints/Main_Tutorial_Demo/EG_FirstRun_Tutorial/EP_FirstRun_WeaponPickupActivateDummy");
const FString TrainingDummyPointPath = TEXT("/Game/Story/EncounterPoints/Main_Tutorial_Demo/EG_FirstRun_Tutorial/EP_FirstRun_TrainingDummyCombo");
const FString TutorialDummyClassPath = TEXT("/Game/DummyTraining/Blueprints/BP_Enemy_DummyTraining.BP_Enemy_DummyTraining_C");
const FName MainRunWeaponActorName = TEXT("WeaponSpawner_MainRun_StartWeapon");
const FName MainRunWeaponActorTag = TEXT("Story.MainRun.StartWeapon");
const FName TutorialWeaponActorName = TEXT("WeaponSpawner_FirstRun_DemoSword");
const FName TutorialWeaponActorTag = TEXT("Story.FirstRun.DemoWeapon");

FString ToObjectPath(const FString& PackagePath)
{
	return PackagePath + TEXT(".") + FPackageName::GetLongPackageAssetName(PackagePath);
}

template<typename AssetT>
AssetT* LoadAssetByPackagePath(const FString& PackagePath)
{
	return Cast<AssetT>(StaticLoadObject(AssetT::StaticClass(), nullptr, *ToObjectPath(PackagePath)));
}

UStoryEncounterPointDA* LoadOrCreateStoryPoint(TArray<UPackage*>& DirtyPackages, bool& bOutCreated)
{
	bOutCreated = false;

	if (UStoryEncounterPointDA* Existing = LoadAssetByPackagePath<UStoryEncounterPointDA>(PickupPointPath))
	{
		return Existing;
	}

	UPackage* Package = CreatePackage(*PickupPointPath);
	if (!Package)
	{
		return nullptr;
	}

	UStoryEncounterPointDA* Point = NewObject<UStoryEncounterPointDA>(
		Package,
		*FPackageName::GetLongPackageAssetName(PickupPointPath),
		RF_Public | RF_Standalone | RF_Transactional);

	if (Point)
	{
		FAssetRegistryModule::AssetCreated(Point);
		Point->MarkPackageDirty();
		DirtyPackages.AddUnique(Package);
		bOutCreated = true;
	}
	return Point;
}

UStoryFlowAsset* LoadOrCreateFlowAsset(TArray<UPackage*>& DirtyPackages, bool& bOutCreated)
{
	bOutCreated = false;

	if (UStoryFlowAsset* Existing = LoadAssetByPackagePath<UStoryFlowAsset>(ActivateFlowPath))
	{
		return Existing;
	}

	// 若路径已存在其他类型的 FA，提示用户手动删除后重试
	if (UFlowAsset* OldAsset = LoadAssetByPackagePath<UFlowAsset>(ActivateFlowPath))
	{
		UE_LOG(LogTemp, Error,
			TEXT("[FirstRunTutorialSpawnerSetup] %s already exists as %s (not UStoryFlowAsset). Delete it in the content browser and re-run."),
			*ActivateFlowPath, *OldAsset->GetClass()->GetName());
		return nullptr;
	}

	UPackage* Package = CreatePackage(*ActivateFlowPath);
	if (!Package)
	{
		return nullptr;
	}

	UStoryFlowAsset* FlowAsset = NewObject<UStoryFlowAsset>(
		Package,
		*FPackageName::GetLongPackageAssetName(ActivateFlowPath),
		RF_Public | RF_Standalone | RF_Transactional);

	if (FlowAsset)
	{
		UFlowGraph::CreateGraph(FlowAsset);
		FAssetRegistryModule::AssetCreated(FlowAsset);
		FlowAsset->MarkPackageDirty();
		DirtyPackages.AddUnique(Package);
		bOutCreated = true;
	}
	return FlowAsset;
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

USNode_ActivateTutorialSpawner* FindActivateNode(UStoryFlowAsset* FlowAsset)
{
	if (!FlowAsset)
	{
		return nullptr;
	}

	for (const TPair<FGuid, UFlowNode*>& Pair : FlowAsset->GetNodes())
	{
		if (USNode_ActivateTutorialSpawner* ActivateNode = Cast<USNode_ActivateTutorialSpawner>(Pair.Value))
		{
			return ActivateNode;
		}
	}
	return nullptr;
}

USNode_SetActorEnabled* FindShowWeaponNode(UStoryFlowAsset* FlowAsset)
{
	if (!FlowAsset)
	{
		return nullptr;
	}

	for (const TPair<FGuid, UFlowNode*>& Pair : FlowAsset->GetNodes())
	{
		if (USNode_SetActorEnabled* SetActorNode = Cast<USNode_SetActorEnabled>(Pair.Value))
		{
			if (SetActorNode->TargetActorTag == TutorialWeaponActorTag || SetActorNode->TargetActorName == TutorialWeaponActorName)
			{
				return SetActorNode;
			}
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

void BreakPinConnection(UEdGraphPin* FromPin, UEdGraphPin* ToPin)
{
	if (!FromPin || !ToPin || !FromPin->LinkedTo.Contains(ToPin))
	{
		return;
	}

	FromPin->Modify();
	ToPin->Modify();
	FromPin->BreakLinkTo(ToPin);
}

USNode_ActivateTutorialSpawner* EnsureActivateNode(
	UStoryFlowAsset* FlowAsset,
	UStoryEncounterPointDA* TrainingDummyPoint,
	TArray<UPackage*>& DirtyPackages)
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
		UE_LOG(LogTemp, Error, TEXT("Flow asset %s has no usable start node."), *ActivateFlowPath);
		return nullptr;
	}

	USNode_SetActorEnabled* ShowWeaponNode = FindShowWeaponNode(FlowAsset);
	UFlowGraphNode* ShowWeaponGraphNode = ShowWeaponNode ? Cast<UFlowGraphNode>(ShowWeaponNode->GetGraphNode()) : nullptr;
	if (!ShowWeaponNode || !ShowWeaponGraphNode)
	{
		ShowWeaponGraphNode = FFlowGraphSchemaAction_NewNode::CreateNode(
			Graph,
			StartGraphNode->GetOutputPin(0),
			USNode_SetActorEnabled::StaticClass(),
			FVector2D(320.f, -120.f),
			false);
		ShowWeaponNode = ShowWeaponGraphNode ? Cast<USNode_SetActorEnabled>(ShowWeaponGraphNode->GetFlowNodeBase()) : nullptr;
	}

	if (ShowWeaponNode)
	{
		ShowWeaponNode->Modify();
		ShowWeaponNode->TargetActorName = TutorialWeaponActorName;
		ShowWeaponNode->TargetActorTag = TutorialWeaponActorTag;
		ShowWeaponNode->bEnabled = true;
	}

	USNode_ActivateTutorialSpawner* ActivateNode = FindActivateNode(FlowAsset);
	UFlowGraphNode* ActivateGraphNode = ActivateNode ? Cast<UFlowGraphNode>(ActivateNode->GetGraphNode()) : nullptr;
	if (!ActivateNode || !ActivateGraphNode)
	{
		ActivateGraphNode = FFlowGraphSchemaAction_NewNode::CreateNode(
			Graph,
			ShowWeaponGraphNode ? ShowWeaponGraphNode->GetOutputPin(0) : StartGraphNode->GetOutputPin(0),
			USNode_ActivateTutorialSpawner::StaticClass(),
			FVector2D(640.f, 0.f),
			false);
		ActivateNode = ActivateGraphNode ? Cast<USNode_ActivateTutorialSpawner>(ActivateGraphNode->GetFlowNodeBase()) : nullptr;
	}

	if (ShowWeaponGraphNode && ActivateGraphNode)
	{
		UEdGraphPin* StartOut = StartGraphNode->GetOutputPin(0);
		UEdGraphPin* ShowWeaponIn = ShowWeaponGraphNode->GetInputPin(0);
		UEdGraphPin* ShowWeaponOut = ShowWeaponGraphNode->GetOutputPin(0);
		UEdGraphPin* ActivateIn = ActivateGraphNode->GetInputPin(0);

		BreakPinConnection(StartOut, ActivateIn);
		EnsurePinConnection(Graph, StartOut, ShowWeaponIn);
		EnsurePinConnection(Graph, ShowWeaponOut, ActivateIn);
	}
	else if (ActivateGraphNode)
	{
		EnsurePinConnection(Graph, StartGraphNode->GetOutputPin(0), ActivateGraphNode->GetInputPin(0));
	}

	if (ActivateNode)
	{
		ActivateNode->Modify();
		ActivateNode->SpawnerActorTag = TEXT("TutorialDummy");
		ActivateNode->EnemyClassOverride = LoadClass<AEnemyCharacterBase>(nullptr, *TutorialDummyClassPath);
		ActivateNode->bSpawnAtSpawnerLocation = true;
		ActivateNode->bCountsForLevelClear = false;
		ActivateNode->bUnregisterFromEnemyAwareness = true;
		ActivateNode->MaxHealthOverride = 0.0f;
		ActivateNode->bRespawnOnDeath = true;
		ActivateNode->RespawnDelay = 2.0f;
		ActivateNode->OnKillEncounterPoint = TrainingDummyPoint;
	}

	FlowAsset->HarvestNodeConnections();
	FlowAsset->MarkPackageDirty();
	DirtyPackages.AddUnique(FlowAsset->GetPackage());
	return ActivateNode;
}

bool ConfigurePickupPoint(UStoryEncounterPointDA* Point, UStoryFlowAsset* ActivateFlow, TArray<UPackage*>& DirtyPackages)
{
	if (!Point || !ActivateFlow)
	{
		return false;
	}

	Point->Modify();
	Point->EncounterId = TEXT("EM_FirstRun_Tutorial");
	Point->NodeId = TEXT("weapon_pickup_activate_dummy");
	Point->DisplayName = FText::FromString(TEXT("Activate tutorial dummy spawner"));
	Point->Kind = EStoryEncounterNodeKind::Object;
	Point->PlayerFacingEvent = FText::FromString(TEXT("Weapon pickup activates the tutorial dummy spawner."));
	Point->FirePolicy = EStoryEncounterFirePolicy::Once;
	Point->Condition.Kind = EStoryEncounterConditionKind::None;
	Point->Actions.Reset();
	Point->NodeEventFlow = ActivateFlow;
	Point->PlacementLevel = TEXT("/Game/Docs/Map/DA_L1_Room/DA_HubRoom_InitialRoom");
	Point->PlacementName = TEXT("WeaponSpawner_FirstRun_DemoSword");
	Point->EditorPosition = FVector2D(360.f, 40.f);
	Point->MarkPackageDirty();
	DirtyPackages.AddUnique(Point->GetPackage());
	return true;
}

FStoryEncounterAction* FindActorEnabledAction(
	UStoryEncounterPointDA* Point,
	FName ActionId,
	FName ReuseKey,
	FName ActorName,
	FName ActorTag,
	bool bEnabled)
{
	if (!Point)
	{
		return nullptr;
	}

	return Point->Actions.FindByPredicate([&](const FStoryEncounterAction& Action)
	{
		if (Action.Kind != EStoryEncounterActionKind::SetActorEnabled)
		{
			return false;
		}

		return Action.ActionId == ActionId
			|| Action.ReuseKey == ReuseKey
			|| (Action.TargetActorTag == ActorTag && Action.bActorEnabled == bEnabled)
			|| (Action.TargetActorName == ActorName && Action.bActorEnabled == bEnabled);
	});
}

bool EnsureActorEnabledAction(
	UStoryEncounterPointDA* Point,
	FName ActionId,
	FName ReuseKey,
	FName ActorName,
	FName ActorTag,
	bool bEnabled,
	TArray<UPackage*>& DirtyPackages)
{
	if (!Point)
	{
		return false;
	}

	Point->Modify();
	FStoryEncounterAction* Action = FindActorEnabledAction(Point, ActionId, ReuseKey, ActorName, ActorTag, bEnabled);
	if (!Action)
	{
		FStoryEncounterAction NewAction;
		NewAction.Kind = EStoryEncounterActionKind::SetActorEnabled;
		NewAction.ActionId = ActionId;
		NewAction.ReuseKey = ReuseKey;
		Point->Actions.Insert(NewAction, 0);
		Action = &Point->Actions[0];
	}

	Action->Kind = EStoryEncounterActionKind::SetActorEnabled;
	Action->ActionId = ActionId;
	Action->ReuseKey = ReuseKey;
	Action->TargetActorName = ActorName;
	Action->TargetActorTag = ActorTag;
	Action->bActorEnabled = bEnabled;

	Point->MarkPackageDirty();
	DirtyPackages.AddUnique(Point->GetPackage());
	return true;
}

bool ConfigureTutorialWeaponVisibility(TArray<UPackage*>& DirtyPackages)
{
	UStoryEncounterPointDA* HubMovePoint = LoadAssetByPackagePath<UStoryEncounterPointDA>(HubMovePointPath);
	if (!HubMovePoint)
	{
		UE_LOG(LogTemp, Warning, TEXT("[FirstRunTutorialSpawnerSetup] Failed to load %s; main-run weapon hide action was not configured."),
			*HubMovePointPath);
		return false;
	}

	return EnsureActorEnabledAction(
		HubMovePoint,
		TEXT("hide_main_run_start_weapon_during_first_run"),
		TEXT("LevelActor.MainRun.StartWeapon.HideDuringFirstRun"),
		MainRunWeaponActorName,
		MainRunWeaponActorTag,
		false,
		DirtyPackages);
}
}

UFirstRunTutorialSpawnerSetupCommandlet::UFirstRunTutorialSpawnerSetupCommandlet()
{
	IsClient = false;
	IsEditor = true;
	IsServer = false;
	LogToConsole = true;
}

int32 UFirstRunTutorialSpawnerSetupCommandlet::Main(const FString& Params)
{
	using namespace FirstRunTutorialSpawnerSetup;

	TArray<UPackage*> DirtyPackages;

	UStoryEncounterPointDA* TrainingDummyPoint = LoadAssetByPackagePath<UStoryEncounterPointDA>(TrainingDummyPointPath);
	if (!TrainingDummyPoint)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to load %s."), *TrainingDummyPointPath);
		return 1;
	}

	bool bCreatedFlow = false;
	UStoryFlowAsset* ActivateFlow = LoadOrCreateFlowAsset(DirtyPackages, bCreatedFlow);
	if (!ActivateFlow)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create/load %s."), *ActivateFlowPath);
		return 1;
	}

	USNode_ActivateTutorialSpawner* ActivateNode = EnsureActivateNode(ActivateFlow, TrainingDummyPoint, DirtyPackages);
	if (!ActivateNode)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to configure Activate Tutorial Spawner node."));
		return 1;
	}

	bool bCreatedPickupPoint = false;
	UStoryEncounterPointDA* PickupPoint = LoadOrCreateStoryPoint(DirtyPackages, bCreatedPickupPoint);
	if (!PickupPoint)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create/load %s."), *PickupPointPath);
		return 1;
	}

	if (!ConfigurePickupPoint(PickupPoint, ActivateFlow, DirtyPackages))
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to configure pickup point."));
		return 1;
	}

	const bool bConfiguredWeaponVisibility = ConfigureTutorialWeaponVisibility(DirtyPackages);

	const bool bSaved = UEditorLoadingAndSavingUtils::SavePackages(DirtyPackages, false);
	if (!bSaved)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to save one or more packages."));
		return 1;
	}

	UE_LOG(LogTemp, Display, TEXT("[FirstRunTutorialSpawnerSetup] %s %s, %s %s, weapon visibility=%s. Story spawn tag=%s enemy=%s killPoint=%s."),
		bCreatedFlow ? TEXT("created") : TEXT("updated"),
		*ActivateFlowPath,
		bCreatedPickupPoint ? TEXT("created") : TEXT("updated"),
		*PickupPointPath,
		bConfiguredWeaponVisibility ? TEXT("configured") : TEXT("not configured"),
		*ActivateNode->SpawnerActorTag.ToString(),
		*GetNameSafe(ActivateNode->EnemyClassOverride.Get()),
		*GetNameSafe(ActivateNode->OnKillEncounterPoint));
	return 0;
}
