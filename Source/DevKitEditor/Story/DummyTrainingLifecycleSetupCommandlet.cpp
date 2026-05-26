#include "Story/DummyTrainingLifecycleSetupCommandlet.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Animation/AnimMontage.h"
#include "BuffFlow/LifecycleFlowAsset.h"
#include "BuffFlow/Nodes/BFNode_Delay.h"
#include "BuffFlow/Nodes/BFNode_FinishLifecycle.h"
#include "BuffFlow/Nodes/BFNode_PlayMontage.h"
#include "BuffFlow/Nodes/BFNode_SpawnEnemyFromContext.h"
#include "Character/EnemyCharacterBase.h"
#include "Component/CharacterDataComponent.h"
#include "Data/AbilityData.h"
#include "Data/EnemyData.h"
#include "FileHelpers.h"
#include "FlowAsset.h"
#include "Graph/FlowGraph.h"
#include "Graph/FlowGraphSchema_Actions.h"
#include "Graph/Nodes/FlowGraphNode.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Misc/PackageName.h"
#include "Nodes/Graph/FlowNode_Start.h"
#include "Story/Flow/Nodes/SNode_ActivateTutorialSpawner.h"
#include "Story/Flow/StoryFlowAsset.h"
#include "UObject/Package.h"

namespace DummyTrainingLifecycleSetup
{
const FString DummyClassPath = TEXT("/Game/DummyTraining/Blueprints/BP_Enemy_DummyTraining.BP_Enemy_DummyTraining_C");
const FString EnemyDataPath = TEXT("/Game/Docs/Data/Enemy/Dummy/DA_Dummy");
const FString AbilityDataPath = TEXT("/Game/Docs/Data/Enemy/Dummy/DA_AbilityMontage_Dummy_01");
const FString TutorialActivateFlowPath = TEXT("/Game/Story/Flows/Tutorial/FA_ActivateTutorialDummySpawner");
const FString SpawnFlowPath = TEXT("/Game/DummyTraining/Flows/FA_DummyTraining_Spawn");
const FString BornMontagePath = TEXT("/Game/DummyTraining/Animations/AM_Dummy_Born.AM_Dummy_Born");
const FString DeathMontagePath = TEXT("/Game/DummyTraining/Animations/AM_Dummy_Death.AM_Dummy_Death");

FString ToObjectPath(const FString& PackagePath)
{
	return PackagePath + TEXT(".") + FPackageName::GetLongPackageAssetName(PackagePath);
}

template<typename AssetT>
AssetT* LoadAssetByPackagePath(const FString& PackagePath)
{
	return Cast<AssetT>(StaticLoadObject(AssetT::StaticClass(), nullptr, *ToObjectPath(PackagePath)));
}

template<typename AssetT>
AssetT* LoadOrCreateFlowAsset(const FString& PackagePath, TArray<UPackage*>& DirtyPackages, bool& bOutCreated)
{
	bOutCreated = false;
	if (AssetT* Existing = LoadAssetByPackagePath<AssetT>(PackagePath))
	{
		return Existing;
	}

	if (UFlowAsset* OldAsset = LoadAssetByPackagePath<UFlowAsset>(PackagePath))
	{
		UE_LOG(LogTemp, Error, TEXT("[DummyTrainingLifecycleSetup] %s already exists as %s."),
			*PackagePath, *GetNameSafe(OldAsset->GetClass()));
		return nullptr;
	}

	UPackage* Package = CreatePackage(*PackagePath);
	if (!Package)
	{
		return nullptr;
	}

	AssetT* FlowAsset = NewObject<AssetT>(
		Package,
		*FPackageName::GetLongPackageAssetName(PackagePath),
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

UFlowNode_Start* FindStartNode(UFlowAsset* FlowAsset)
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

UEdGraphPin* FindGraphPin(UFlowNode* Node, const FName PinName, const EEdGraphPinDirection Direction)
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

UEdGraphPin* FirstGraphPin(UFlowNode* Node, const EEdGraphPinDirection Direction)
{
	UFlowGraphNode* GraphNode = Node ? Cast<UFlowGraphNode>(Node->GetGraphNode()) : nullptr;
	if (!GraphNode)
	{
		return nullptr;
	}

	for (UEdGraphPin* Pin : GraphNode->Pins)
	{
		if (Pin && Pin->Direction == Direction)
		{
			return Pin;
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

bool ArePinsConnected(UFlowNode* FromNode, const FName FromPinName, UFlowNode* ToNode, const FName ToPinName)
{
	UEdGraphPin* FromPin = FindGraphPin(FromNode, FromPinName, EGPD_Output);
	UEdGraphPin* ToPin = FindGraphPin(ToNode, ToPinName, EGPD_Input);
	return FromPin && ToPin && FromPin->LinkedTo.Contains(ToPin);
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

UFlowNode* CreateNodeFromPin(UFlowGraph* FlowGraph, UEdGraphPin* FromPin, UClass* NodeClass, const FVector2D& Location)
{
	UFlowGraphNode* GraphNode = FFlowGraphSchemaAction_NewNode::CreateNode(FlowGraph, FromPin, NodeClass, Location, false);
	return GraphNode ? Cast<UFlowNode>(GraphNode->GetFlowNodeBase()) : nullptr;
}

bool EnsureGraph(UFlowAsset* FlowAsset)
{
	if (!FlowAsset)
	{
		return false;
	}

	if (!FlowAsset->GetGraph())
	{
		UFlowGraph::CreateGraph(FlowAsset);
	}

	return FlowAsset->GetGraph() && FindStartNode(FlowAsset);
}

template<typename NodeT>
NodeT* FindFirstNodeOfType(UFlowAsset* FlowAsset)
{
	if (!FlowAsset)
	{
		return nullptr;
	}

	for (const TPair<FGuid, UFlowNode*>& Pair : FlowAsset->GetNodes())
	{
		if (NodeT* Node = Cast<NodeT>(Pair.Value))
		{
			return Node;
		}
	}
	return nullptr;
}

UBFNode_PlayMontage* FindPlayMontageNode(UFlowAsset* FlowAsset, UAnimMontage* Montage)
{
	if (!FlowAsset || !Montage)
	{
		return nullptr;
	}

	for (const TPair<FGuid, UFlowNode*>& Pair : FlowAsset->GetNodes())
	{
		UBFNode_PlayMontage* MontageNode = Cast<UBFNode_PlayMontage>(Pair.Value);
		if (MontageNode && MontageNode->Montage == Montage)
		{
			return MontageNode;
		}
	}
	return nullptr;
}

bool IsConnectedToFinishLifecycle(UFlowAsset* FlowAsset, UFlowNode* FromNode, const FName FromPinName)
{
	if (!FlowAsset || !FromNode)
	{
		return false;
	}

	for (const TPair<FGuid, UFlowNode*>& Pair : FlowAsset->GetNodes())
	{
		if (UBFNode_FinishLifecycle* FinishNode = Cast<UBFNode_FinishLifecycle>(Pair.Value))
		{
			if (ArePinsConnected(FromNode, FromPinName, FinishNode, TEXT("In")))
			{
				return true;
			}
		}
	}
	return false;
}

bool VerifySpawnFlowGraph(USpawnLifecycleFlowAsset* FlowAsset, UAnimMontage* BornMontage)
{
	UFlowNode_Start* StartNode = FindStartNode(FlowAsset);
	UBFNode_SpawnEnemyFromContext* SpawnNode = FindFirstNodeOfType<UBFNode_SpawnEnemyFromContext>(FlowAsset);
	UBFNode_PlayMontage* BornNode = FindPlayMontageNode(FlowAsset, BornMontage);
	UBFNode_Delay* DelayNode = FindFirstNodeOfType<UBFNode_Delay>(FlowAsset);

	return StartNode
		&& SpawnNode
		&& BornNode
		&& DelayNode
		&& BornNode->TargetSelector == EBFTargetSelector::LifecycleTarget
		&& ArePinsConnected(StartNode, TEXT("Out"), SpawnNode, TEXT("In"))
		&& ArePinsConnected(SpawnNode, TEXT("Spawned"), BornNode, TEXT("In"))
		&& ArePinsConnected(BornNode, TEXT("Out"), DelayNode, TEXT("In"))
		&& IsConnectedToFinishLifecycle(FlowAsset, DelayNode, TEXT("Completed"))
		&& IsConnectedToFinishLifecycle(FlowAsset, SpawnNode, TEXT("Failed"));
}

bool ConfigureSpawnFlow(USpawnLifecycleFlowAsset* FlowAsset, UAnimMontage* BornMontage, TArray<UPackage*>& DirtyPackages)
{
	if (!FlowAsset || !BornMontage || !EnsureGraph(FlowAsset))
	{
		return false;
	}

	FlowAsset->Modify();
	ClearNonEntryNodes(FlowAsset);

	UFlowGraph* FlowGraph = Cast<UFlowGraph>(FlowAsset->GetGraph());
	UFlowNode_Start* StartNode = FindStartNode(FlowAsset);
	UEdGraphPin* StartOut = FindGraphPin(StartNode, TEXT("Out"), EGPD_Output);
	if (!StartOut)
	{
		StartOut = FirstGraphPin(StartNode, EGPD_Output);
	}

	UBFNode_SpawnEnemyFromContext* SpawnNode = Cast<UBFNode_SpawnEnemyFromContext>(
		CreateNodeFromPin(FlowGraph, StartOut, UBFNode_SpawnEnemyFromContext::StaticClass(), FVector2D(320.f, 0.f)));
	UBFNode_PlayMontage* BornNode = Cast<UBFNode_PlayMontage>(
		CreateNodeFromPin(FlowGraph, FindGraphPin(SpawnNode, TEXT("Spawned"), EGPD_Output), UBFNode_PlayMontage::StaticClass(), FVector2D(640.f, -80.f)));
	UBFNode_Delay* DelayNode = Cast<UBFNode_Delay>(
		CreateNodeFromPin(FlowGraph, FindGraphPin(BornNode, TEXT("Out"), EGPD_Output), UBFNode_Delay::StaticClass(), FVector2D(960.f, -80.f)));
	UBFNode_FinishLifecycle* FinishNode = Cast<UBFNode_FinishLifecycle>(
		CreateNodeFromPin(FlowGraph, FindGraphPin(DelayNode, TEXT("Completed"), EGPD_Output), UBFNode_FinishLifecycle::StaticClass(), FVector2D(1280.f, -80.f)));
	UBFNode_FinishLifecycle* FailedFinishNode = Cast<UBFNode_FinishLifecycle>(
		CreateNodeFromPin(FlowGraph, FindGraphPin(SpawnNode, TEXT("Failed"), EGPD_Output), UBFNode_FinishLifecycle::StaticClass(), FVector2D(640.f, 160.f)));

	if (!SpawnNode || !BornNode || !DelayNode || !FinishNode || !FailedFinishNode)
	{
		return false;
	}

	BornNode->Modify();
	BornNode->Montage = BornMontage;
	BornNode->TargetSelector = EBFTargetSelector::LifecycleTarget;
	BornNode->PlayRate = 1.f;

	DelayNode->Modify();
	DelayNode->Duration = FFlowDataPinInputProperty_Float(FMath::Max(0.f, BornMontage->GetPlayLength()));

	const bool bConnected = EnsurePinConnection(FlowGraph, StartOut, FindGraphPin(SpawnNode, TEXT("In"), EGPD_Input))
		&& EnsurePinConnection(FlowGraph, FindGraphPin(SpawnNode, TEXT("Spawned"), EGPD_Output), FindGraphPin(BornNode, TEXT("In"), EGPD_Input))
		&& EnsurePinConnection(FlowGraph, FindGraphPin(BornNode, TEXT("Out"), EGPD_Output), FindGraphPin(DelayNode, TEXT("In"), EGPD_Input))
		&& EnsurePinConnection(FlowGraph, FindGraphPin(DelayNode, TEXT("Completed"), EGPD_Output), FindGraphPin(FinishNode, TEXT("In"), EGPD_Input))
		&& EnsurePinConnection(FlowGraph, FindGraphPin(SpawnNode, TEXT("Failed"), EGPD_Output), FindGraphPin(FailedFinishNode, TEXT("In"), EGPD_Input));
	if (!bConnected)
	{
		UE_LOG(LogTemp, Error, TEXT("[DummyTrainingLifecycleSetup] Failed to connect spawn lifecycle flow graph."));
		return false;
	}

	FlowAsset->HarvestNodeConnections();
	if (!VerifySpawnFlowGraph(FlowAsset, BornMontage))
	{
		UE_LOG(LogTemp, Error, TEXT("[DummyTrainingLifecycleSetup] Spawn lifecycle flow graph verification failed."));
		return false;
	}

	FlowAsset->MarkPackageDirty();
	DirtyPackages.AddUnique(FlowAsset->GetPackage());
	return true;
}

USNode_ActivateTutorialSpawner* FindActivateTutorialSpawnerNode(UStoryFlowAsset* FlowAsset)
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

bool ConfigureTutorialSpawnerFlow(TSubclassOf<AEnemyCharacterBase> DummyClass, TArray<UPackage*>& DirtyPackages)
{
	UStoryFlowAsset* FlowAsset = LoadAssetByPackagePath<UStoryFlowAsset>(TutorialActivateFlowPath);
	USNode_ActivateTutorialSpawner* ActivateNode = FindActivateTutorialSpawnerNode(FlowAsset);
	if (!FlowAsset || !ActivateNode || !DummyClass)
	{
		UE_LOG(LogTemp, Error, TEXT("[DummyTrainingLifecycleSetup] Failed to load tutorial activation flow/node/class."));
		return false;
	}

	ActivateNode->Modify();
	ActivateNode->EnemyClassOverride = DummyClass;
	FlowAsset->HarvestNodeConnections();
	FlowAsset->MarkPackageDirty();
	DirtyPackages.AddUnique(FlowAsset->GetPackage());
	return true;
}

bool ConfigureDummyBlueprint(TSubclassOf<AEnemyCharacterBase> DummyClass, UEnemyData* EnemyData, TArray<UPackage*>& DirtyPackages)
{
	if (!DummyClass || !EnemyData)
	{
		return false;
	}

	AEnemyCharacterBase* CDO = DummyClass->GetDefaultObject<AEnemyCharacterBase>();
	UCharacterDataComponent* CharacterDataComponent = CDO ? CDO->FindComponentByClass<UCharacterDataComponent>() : nullptr;
	if (!CDO || !CharacterDataComponent)
	{
		UE_LOG(LogTemp, Error, TEXT("[DummyTrainingLifecycleSetup] Dummy class has no usable CDO/CharacterDataComponent."));
		return false;
	}

	CharacterDataComponent->Modify();
	CharacterDataComponent->SetCharacterData(EnemyData);
	CDO->MarkPackageDirty();
	DirtyPackages.AddUnique(DummyClass->GetPackage());

	if (UBlueprint* Blueprint = Cast<UBlueprint>(DummyClass->ClassGeneratedBy))
	{
		FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
		DirtyPackages.AddUnique(Blueprint->GetPackage());
	}

	return true;
}

}

UDummyTrainingLifecycleSetupCommandlet::UDummyTrainingLifecycleSetupCommandlet()
{
	IsClient = false;
	IsEditor = true;
	IsServer = false;
	LogToConsole = true;
}

int32 UDummyTrainingLifecycleSetupCommandlet::Main(const FString& Params)
{
	using namespace DummyTrainingLifecycleSetup;

	TArray<UPackage*> DirtyPackages;

	TSubclassOf<AEnemyCharacterBase> DummyClass = LoadClass<AEnemyCharacterBase>(nullptr, *DummyClassPath);
	UEnemyData* EnemyData = LoadAssetByPackagePath<UEnemyData>(EnemyDataPath);
	UAbilityData* AbilityData = LoadAssetByPackagePath<UAbilityData>(AbilityDataPath);
	UAnimMontage* BornMontage = LoadObject<UAnimMontage>(nullptr, *BornMontagePath);
	UAnimMontage* DeathMontage = LoadObject<UAnimMontage>(nullptr, *DeathMontagePath);

	if (!DummyClass || !EnemyData || !AbilityData || !BornMontage || !DeathMontage)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[DummyTrainingLifecycleSetup] Missing required asset. Class=%s EnemyData=%s AbilityData=%s Born=%s Death=%s"),
			*GetNameSafe(DummyClass.Get()),
			*GetNameSafe(EnemyData),
			*GetNameSafe(AbilityData),
			*GetNameSafe(BornMontage),
			*GetNameSafe(DeathMontage));
		return 1;
	}

	bool bCreatedSpawnFlow = false;
	USpawnLifecycleFlowAsset* SpawnFlow = LoadOrCreateFlowAsset<USpawnLifecycleFlowAsset>(SpawnFlowPath, DirtyPackages, bCreatedSpawnFlow);
	if (!SpawnFlow)
	{
		return 1;
	}

	if (!ConfigureSpawnFlow(SpawnFlow, BornMontage, DirtyPackages)
		|| !ConfigureTutorialSpawnerFlow(DummyClass, DirtyPackages))
	{
		return 1;
	}

	EnemyData->Modify();
	EnemyData->EnemyClass = DummyClass;
	EnemyData->AbilityData = AbilityData;
	EnemyData->SpawnLifecycleFlow = SpawnFlow;
	EnemyData->MarkPackageDirty();
	DirtyPackages.AddUnique(EnemyData->GetPackage());

	AbilityData->Modify();
	const FGameplayTag DeadTag = FGameplayTag::RequestGameplayTag(TEXT("Action.Dead"), false);
	if (DeadTag.IsValid())
	{
		FPassiveActionData& DeadData = AbilityData->PassiveMap.FindOrAdd(DeadTag);
		DeadData.Montage = DeathMontage;
	}
	AbilityData->MarkPackageDirty();
	DirtyPackages.AddUnique(AbilityData->GetPackage());

	if (!ConfigureDummyBlueprint(DummyClass, EnemyData, DirtyPackages))
	{
		return 1;
	}

	const bool bSaved = UEditorLoadingAndSavingUtils::SavePackages(DirtyPackages, false);
	if (!bSaved)
	{
		UE_LOG(LogTemp, Error, TEXT("[DummyTrainingLifecycleSetup] Failed to save one or more packages."));
		return 1;
	}

	if (EnemyData->EnemyClass != DummyClass
		|| EnemyData->AbilityData != AbilityData
		|| EnemyData->SpawnLifecycleFlow != SpawnFlow
		|| !VerifySpawnFlowGraph(SpawnFlow, BornMontage)
		|| (DeadTag.IsValid() && AbilityData->GetPassiveAbility(DeadTag).Montage != DeathMontage))
	{
		UE_LOG(LogTemp, Error, TEXT("[DummyTrainingLifecycleSetup] Verification failed after save."));
		return 1;
	}

	UE_LOG(LogTemp, Display,
		TEXT("[DummyTrainingLifecycleSetup] Configured dummy lifecycle. Dummy=%s EnemyData=%s SpawnFlow=%s(%s) Born=%s Death=%s"),
		*GetNameSafe(DummyClass.Get()),
		*GetNameSafe(EnemyData),
		*GetNameSafe(SpawnFlow),
		bCreatedSpawnFlow ? TEXT("created") : TEXT("updated"),
		*GetNameSafe(BornMontage),
		*GetNameSafe(DeathMontage));
	return 0;
}
