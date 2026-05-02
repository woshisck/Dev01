#include "DevKitEditor/EnemyAI/EnemyAITemplateGeneratorCommandlet.h"

#include "AI/BTDecorator_EnemyAIState.h"
#include "AI/BTService_UpdateEnemyAwareness.h"
#include "AI/BTService_UpdateEnemyCombatMove.h"
#include "AI/BTTask_EnemyAttackByProfile.h"
#include "AI/BTTask_EnemyPatrolWait.h"
#include "AI/BTTask_UpdateEnemyPatrolTarget.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardData.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Bool.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Enum.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Float.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"
#include "BehaviorTree/Composites/BTComposite_Selector.h"
#include "BehaviorTree/Composites/BTComposite_Sequence.h"
#include "BehaviorTree/Tasks/BTTask_BlackboardBase.h"
#include "BehaviorTree/Tasks/BTTask_MoveTo.h"
#include "BehaviorTree/Tasks/BTTask_Wait.h"
#include "BehaviorTreeGraph.h"
#include "BehaviorTreeGraphNode.h"
#include "BehaviorTreeGraphNode_Composite.h"
#include "BehaviorTreeGraphNode_Decorator.h"
#include "BehaviorTreeGraphNode_Root.h"
#include "BehaviorTreeGraphNode_Service.h"
#include "BehaviorTreeGraphNode_Task.h"
#include "Data/EnemyData.h"
#include "EdGraph/EdGraphPin.h"
#include "EdGraphSchema_BehaviorTree.h"
#include "FileHelpers.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"
#include "UObject/UnrealType.h"

namespace EnemyAITemplateGenerator
{
	const FString BlackboardPath = TEXT("/Game/Code/Enemy/AI/BlackBoard/BB_Enemy_DefaultMelee");
	const FString BehaviorTreePath = TEXT("/Game/Code/Enemy/AI/Behaviour/BT_Enemy_DefaultMelee");
	const FString RatDataPath = TEXT("/Game/Docs/Data/Enemy/Rat/DA_Rat");
	const FString RottenGuardDataPath = TEXT("/Game/Docs/Data/Enemy/RottenGuard/DA_RottenGuard");

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
	T* LoadAssetByPackagePath(const FString& PackagePath, uint32 LoadFlags = LOAD_None)
	{
		if (T* Existing = FindObject<T>(nullptr, *ToObjectPath(PackagePath)))
		{
			return Existing;
		}

		if (!PackageExists(PackagePath))
		{
			return nullptr;
		}

		return Cast<T>(StaticLoadObject(T::StaticClass(), nullptr, *ToObjectPath(PackagePath), nullptr, LoadFlags));
	}

	template <typename T>
	T* CreateOrLoadAsset(const FString& PackagePath, bool bDryRun, TArray<FString>& ReportLines, TArray<UPackage*>& DirtyPackages)
	{
		if (T* Existing = LoadAssetByPackagePath<T>(PackagePath))
		{
			ReportLines.Add(FString::Printf(TEXT("- Found `%s`."), *PackagePath));
			return Existing;
		}

		ReportLines.Add(FString::Printf(TEXT("- %s `%s`."), bDryRun ? TEXT("Would create") : TEXT("Created"), *PackagePath));
		if (bDryRun)
		{
			return nullptr;
		}

		UPackage* Package = CreatePackage(*PackagePath);
		const FName AssetName(*FPackageName::GetLongPackageAssetName(PackagePath));
		T* Asset = NewObject<T>(Package, AssetName, RF_Public | RF_Standalone | RF_Transactional);
		FAssetRegistryModule::AssetCreated(Asset);
		Asset->MarkPackageDirty();
		DirtyPackages.AddUnique(Package);
		return Asset;
	}

	template <typename T>
	T* EnsureBlackboardKey(UBlackboardData& Blackboard, FName KeyName)
	{
		if (T* NewKey = Blackboard.UpdatePersistentKey<T>(KeyName))
		{
			return NewKey;
		}

		const FBlackboard::FKey KeyID = Blackboard.GetKeyID(KeyName);
		const FBlackboardEntry* Entry = Blackboard.GetKey(KeyID);
		return Entry ? Cast<T>(Entry->KeyType) : nullptr;
	}

	void ConfigureBlackboard(UBlackboardData& Blackboard)
	{
		if (UBlackboardKeyType_Enum* StateKey = EnsureBlackboardKey<UBlackboardKeyType_Enum>(Blackboard, TEXT("EnemyAIState")))
		{
			StateKey->EnumType = StaticEnum<EEnemyAIState>();
			StateKey->EnumName = TEXT("/Script/DevKit.EEnemyAIState");
		}

		if (UBlackboardKeyType_Object* TargetKey = EnsureBlackboardKey<UBlackboardKeyType_Object>(Blackboard, TEXT("TargetActor")))
		{
			TargetKey->BaseClass = AActor::StaticClass();
		}

		EnsureBlackboardKey<UBlackboardKeyType_Vector>(Blackboard, TEXT("LastKnownTargetLocation"));
		EnsureBlackboardKey<UBlackboardKeyType_Vector>(Blackboard, TEXT("PatrolOriginLocation"));
		EnsureBlackboardKey<UBlackboardKeyType_Vector>(Blackboard, TEXT("PatrolTargetLocation"));
		EnsureBlackboardKey<UBlackboardKeyType_Vector>(Blackboard, TEXT("MoveTargetLocation"));
		EnsureBlackboardKey<UBlackboardKeyType_Float>(Blackboard, TEXT("DistanceToTarget"));
		EnsureBlackboardKey<UBlackboardKeyType_Bool>(Blackboard, TEXT("bInAttackRange"));
		EnsureBlackboardKey<UBlackboardKeyType_Float>(Blackboard, TEXT("AcceptanceRadius"));
		EnsureBlackboardKey<UBlackboardKeyType_Float>(Blackboard, TEXT("AlertExpireTime"));
		EnsureBlackboardKey<UBlackboardKeyType_Float>(Blackboard, TEXT("LastSeenTargetTime"));

		Blackboard.UpdateKeyIDs();
		Blackboard.MarkPackageDirty();
	}

	void SetMoveToBlackboardKey(UBTTask_MoveTo& MoveToTask, FName KeyName)
	{
		FStructProperty* BlackboardKeyProperty = FindFProperty<FStructProperty>(UBTTask_BlackboardBase::StaticClass(), TEXT("BlackboardKey"));
		if (!BlackboardKeyProperty)
		{
			return;
		}

		FBlackboardKeySelector* Selector = BlackboardKeyProperty->ContainerPtrToValuePtr<FBlackboardKeySelector>(&MoveToTask);
		if (Selector)
		{
			Selector->SelectedKeyName = KeyName;
		}
	}

	template <typename GraphNodeType, typename NodeInstanceType>
	GraphNodeType* AddNode(UBehaviorTree& BehaviorTree, UBehaviorTreeGraph& Graph, int32 X, int32 Y)
	{
		GraphNodeType* GraphNode = NewObject<GraphNodeType>(&Graph);
		GraphNode->NodeInstance = NewObject<NodeInstanceType>(&BehaviorTree);
		GraphNode->SetFlags(RF_Transactional);
		GraphNode->NodeInstance->SetFlags(RF_Transactional);
		GraphNode->NodePosX = X;
		GraphNode->NodePosY = Y;
		GraphNode->CreateNewGuid();
		GraphNode->UpdateNodeClassData();
		GraphNode->PostPlacedNewNode();
		GraphNode->AllocateDefaultPins();
		Graph.AddNode(GraphNode, false, false);
		return GraphNode;
	}

	void Connect(UBehaviorTreeGraphNode* Parent, UBehaviorTreeGraphNode* Child)
	{
		if (!Parent || !Child || !Parent->GetOutputPin() || !Child->GetInputPin())
		{
			return;
		}

		Parent->GetOutputPin()->MakeLinkTo(Child->GetInputPin());
	}

	template <typename ServiceType>
	void AddService(UBehaviorTree& BehaviorTree, UBehaviorTreeGraph& Graph, UBehaviorTreeGraphNode& Parent)
	{
		UBehaviorTreeGraphNode_Service* ServiceNode = NewObject<UBehaviorTreeGraphNode_Service>(&Graph);
		ServiceNode->NodeInstance = NewObject<ServiceType>(&BehaviorTree);
		ServiceNode->SetFlags(RF_Transactional);
		ServiceNode->NodeInstance->SetFlags(RF_Transactional);
		ServiceNode->CreateNewGuid();
		ServiceNode->UpdateNodeClassData();
		Parent.AddSubNode(ServiceNode, &Graph);
	}

	void AddStateDecorator(UBehaviorTree& BehaviorTree, UBehaviorTreeGraph& Graph, UBehaviorTreeGraphNode& DecoratedNode, EEnemyAIState State)
	{
		UBehaviorTreeGraphNode_Decorator* DecoratorNode = NewObject<UBehaviorTreeGraphNode_Decorator>(&Graph);
		UBTDecorator_EnemyAIState* Decorator = NewObject<UBTDecorator_EnemyAIState>(&BehaviorTree);
		Decorator->SetRequiredState(State);
		DecoratorNode->NodeInstance = Decorator;
		DecoratorNode->SetFlags(RF_Transactional);
		Decorator->SetFlags(RF_Transactional);
		DecoratorNode->CreateNewGuid();
		DecoratorNode->UpdateNodeClassData();
		DecoratedNode.AddSubNode(DecoratorNode, &Graph);
	}

	UBehaviorTreeGraphNode_Root* FindOrCreateRootNode(UBehaviorTreeGraph& Graph, UBlackboardData& Blackboard)
	{
		for (UEdGraphNode* Node : Graph.Nodes)
		{
			if (UBehaviorTreeGraphNode_Root* RootNode = Cast<UBehaviorTreeGraphNode_Root>(Node))
			{
				RootNode->BlackboardAsset = &Blackboard;
				RootNode->UpdateBlackboard();
				return RootNode;
			}
		}

		UBehaviorTreeGraphNode_Root* RootNode = NewObject<UBehaviorTreeGraphNode_Root>(&Graph);
		RootNode->BlackboardAsset = &Blackboard;
		RootNode->NodePosX = 0;
		RootNode->NodePosY = 0;
		RootNode->CreateNewGuid();
		RootNode->PostPlacedNewNode();
		RootNode->AllocateDefaultPins();
		RootNode->BlackboardAsset = &Blackboard;
		RootNode->UpdateBlackboard();
		Graph.AddNode(RootNode, false, false);
		return RootNode;
	}

	void RebuildBehaviorTreeGraph(UBehaviorTree& BehaviorTree, UBlackboardData& Blackboard)
	{
		BehaviorTree.BlackboardAsset = &Blackboard;

		UBehaviorTreeGraph* Graph = Cast<UBehaviorTreeGraph>(BehaviorTree.BTGraph);
		if (!Graph)
		{
			Graph = NewObject<UBehaviorTreeGraph>(&BehaviorTree, TEXT("BTGraph"), RF_Transactional);
			BehaviorTree.BTGraph = Graph;
		}

		Graph->Modify();
		Graph->Nodes.Reset();

		if (const UEdGraphSchema* Schema = Graph->GetSchema())
		{
			Schema->CreateDefaultNodesForGraph(*Graph);
		}

		UBehaviorTreeGraphNode_Root* RootNode = FindOrCreateRootNode(*Graph, Blackboard);
		RootNode->BlackboardAsset = &Blackboard;
		RootNode->UpdateBlackboard();

		UBehaviorTreeGraphNode_Composite* MainSelector = AddNode<UBehaviorTreeGraphNode_Composite, UBTComposite_Selector>(BehaviorTree, *Graph, 0, 160);
		AddService<UBTService_UpdateEnemyAwareness>(BehaviorTree, *Graph, *MainSelector);
		AddService<UBTService_UpdateEnemyCombatMove>(BehaviorTree, *Graph, *MainSelector);
		Connect(RootNode, MainSelector);

		UBehaviorTreeGraphNode_Composite* CombatSelector = AddNode<UBehaviorTreeGraphNode_Composite, UBTComposite_Selector>(BehaviorTree, *Graph, -450, 360);
		AddStateDecorator(BehaviorTree, *Graph, *CombatSelector, EEnemyAIState::Combat);
		Connect(MainSelector, CombatSelector);

		UBehaviorTreeGraphNode_Task* AttackTask = AddNode<UBehaviorTreeGraphNode_Task, UBTTask_EnemyAttackByProfile>(BehaviorTree, *Graph, -650, 560);
		Connect(CombatSelector, AttackTask);

		UBehaviorTreeGraphNode_Task* CombatMoveTask = AddNode<UBehaviorTreeGraphNode_Task, UBTTask_MoveTo>(BehaviorTree, *Graph, -250, 560);
		if (UBTTask_MoveTo* MoveTo = Cast<UBTTask_MoveTo>(CombatMoveTask->NodeInstance))
		{
			SetMoveToBlackboardKey(*MoveTo, TEXT("MoveTargetLocation"));
			MoveTo->AcceptableRadius = 70.0f;
			MoveTo->bObserveBlackboardValue = true;
			MoveTo->bAllowPartialPath = true;
			MoveTo->bProjectGoalLocation = true;
		}
		Connect(CombatSelector, CombatMoveTask);

		UBehaviorTreeGraphNode_Composite* AlertSequence = AddNode<UBehaviorTreeGraphNode_Composite, UBTComposite_Sequence>(BehaviorTree, *Graph, 0, 360);
		AddStateDecorator(BehaviorTree, *Graph, *AlertSequence, EEnemyAIState::Alert);
		Connect(MainSelector, AlertSequence);

		UBehaviorTreeGraphNode_Task* AlertMoveTask = AddNode<UBehaviorTreeGraphNode_Task, UBTTask_MoveTo>(BehaviorTree, *Graph, -80, 560);
		if (UBTTask_MoveTo* MoveTo = Cast<UBTTask_MoveTo>(AlertMoveTask->NodeInstance))
		{
			SetMoveToBlackboardKey(*MoveTo, TEXT("LastKnownTargetLocation"));
			MoveTo->AcceptableRadius = 100.0f;
			MoveTo->bObserveBlackboardValue = true;
			MoveTo->bAllowPartialPath = true;
			MoveTo->bProjectGoalLocation = true;
		}
		Connect(AlertSequence, AlertMoveTask);

		UBehaviorTreeGraphNode_Task* AlertWaitTask = AddNode<UBehaviorTreeGraphNode_Task, UBTTask_Wait>(BehaviorTree, *Graph, 120, 560);
		if (UBTTask_Wait* Wait = Cast<UBTTask_Wait>(AlertWaitTask->NodeInstance))
		{
			Wait->WaitTime = 0.3f;
			Wait->RandomDeviation = 0.15f;
		}
		Connect(AlertSequence, AlertWaitTask);

		UBehaviorTreeGraphNode_Composite* PatrolSequence = AddNode<UBehaviorTreeGraphNode_Composite, UBTComposite_Sequence>(BehaviorTree, *Graph, 450, 360);
		AddStateDecorator(BehaviorTree, *Graph, *PatrolSequence, EEnemyAIState::Patrol);
		Connect(MainSelector, PatrolSequence);

		UBehaviorTreeGraphNode_Task* PatrolTargetTask = AddNode<UBehaviorTreeGraphNode_Task, UBTTask_UpdateEnemyPatrolTarget>(BehaviorTree, *Graph, 250, 560);
		Connect(PatrolSequence, PatrolTargetTask);

		UBehaviorTreeGraphNode_Task* PatrolMoveTask = AddNode<UBehaviorTreeGraphNode_Task, UBTTask_MoveTo>(BehaviorTree, *Graph, 450, 560);
		if (UBTTask_MoveTo* MoveTo = Cast<UBTTask_MoveTo>(PatrolMoveTask->NodeInstance))
		{
			SetMoveToBlackboardKey(*MoveTo, TEXT("PatrolTargetLocation"));
			MoveTo->AcceptableRadius = 120.0f;
			MoveTo->bAllowPartialPath = true;
			MoveTo->bProjectGoalLocation = true;
		}
		Connect(PatrolSequence, PatrolMoveTask);

		UBehaviorTreeGraphNode_Task* PatrolWaitTask = AddNode<UBehaviorTreeGraphNode_Task, UBTTask_EnemyPatrolWait>(BehaviorTree, *Graph, 650, 560);
		Connect(PatrolSequence, PatrolWaitTask);

		Graph->UpdateAsset();
		BehaviorTree.MarkPackageDirty();
	}

	void AssignBehaviorTreeToEnemyData(const FString& EnemyDataPath, UBehaviorTree* BehaviorTree, bool bDryRun, TArray<FString>& ReportLines, TArray<UPackage*>& DirtyPackages)
	{
		if (bDryRun)
		{
			ReportLines.Add(FString::Printf(TEXT("- %s `%s`.BehaviorTree -> `%s`."),
				PackageExists(EnemyDataPath) ? TEXT("Would set") : TEXT("Missing enemy DA"),
				*EnemyDataPath,
				*BehaviorTreePath));
			return;
		}

		const uint32 EnemyDataLoadFlags = LOAD_NoWarn | LOAD_NoVerify | LOAD_DisableDependencyPreloading | LOAD_DisableCompileOnLoad;
		UEnemyData* EnemyData = LoadAssetByPackagePath<UEnemyData>(EnemyDataPath, EnemyDataLoadFlags);
		if (!EnemyData)
		{
			ReportLines.Add(FString::Printf(TEXT("- Missing enemy DA `%s`."), *EnemyDataPath));
			return;
		}

		ReportLines.Add(FString::Printf(TEXT("- %s `%s`.BehaviorTree -> `%s`."),
			bDryRun ? TEXT("Would set") : TEXT("Set"),
			*EnemyDataPath,
			*BehaviorTreePath));

		if (!BehaviorTree)
		{
			return;
		}

		EnemyData->Modify();
		EnemyData->BehaviorTree = BehaviorTree;
		EnemyData->MarkPackageDirty();
		DirtyPackages.AddUnique(EnemyData->GetPackage());
	}
}

UEnemyAITemplateGeneratorCommandlet::UEnemyAITemplateGeneratorCommandlet()
{
	IsClient = false;
	IsEditor = true;
	LogToConsole = true;
}

int32 UEnemyAITemplateGeneratorCommandlet::Main(const FString& Params)
{
	using namespace EnemyAITemplateGenerator;

	const bool bDryRun = Params.Contains(TEXT("DryRun"), ESearchCase::IgnoreCase);
	const bool bPresetDefaultMelee = Params.Contains(TEXT("Preset=DefaultMelee"), ESearchCase::IgnoreCase)
		|| !Params.Contains(TEXT("Preset="), ESearchCase::IgnoreCase);

	TArray<FString> ReportLines;
	TArray<UPackage*> DirtyPackages;
	ReportLines.Add(TEXT("# Enemy AI Template Generator Report"));
	ReportLines.Add(FString::Printf(TEXT("- Mode: %s"), bDryRun ? TEXT("DryRun") : TEXT("Apply")));
	ReportLines.Add(FString::Printf(TEXT("- Preset: %s"), bPresetDefaultMelee ? TEXT("DefaultMelee") : TEXT("Unsupported")));
	ReportLines.Add(TEXT(""));

	if (!bPresetDefaultMelee)
	{
		ReportLines.Add(TEXT("- Unsupported preset. Use `-Preset=DefaultMelee`."));
	}
	else
	{
		ReportLines.Add(TEXT("## Blackboard"));
		UBlackboardData* Blackboard = CreateOrLoadAsset<UBlackboardData>(BlackboardPath, bDryRun, ReportLines, DirtyPackages);
		if (!bDryRun && Blackboard)
		{
			ConfigureBlackboard(*Blackboard);
			DirtyPackages.AddUnique(Blackboard->GetPackage());
			ReportLines.Add(TEXT("- Ensured default melee blackboard keys."));
		}

		ReportLines.Add(TEXT(""));
		ReportLines.Add(TEXT("## Behavior Tree"));
		UBehaviorTree* BehaviorTree = CreateOrLoadAsset<UBehaviorTree>(BehaviorTreePath, bDryRun, ReportLines, DirtyPackages);
		if (!bDryRun && BehaviorTree && Blackboard)
		{
			RebuildBehaviorTreeGraph(*BehaviorTree, *Blackboard);
			DirtyPackages.AddUnique(BehaviorTree->GetPackage());
			ReportLines.Add(TEXT("- Rebuilt visual graph and runtime tree."));
		}

		ReportLines.Add(TEXT(""));
		ReportLines.Add(TEXT("## Enemy Data"));
		AssignBehaviorTreeToEnemyData(RatDataPath, BehaviorTree, bDryRun, ReportLines, DirtyPackages);
		AssignBehaviorTreeToEnemyData(RottenGuardDataPath, BehaviorTree, bDryRun, ReportLines, DirtyPackages);
	}

	if (!bDryRun && DirtyPackages.Num() > 0)
	{
		UEditorLoadingAndSavingUtils::SavePackages(DirtyPackages, false);
	}

	const FString ReportPath = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("EnemyAITemplateGeneratorReport.md"));
	FFileHelper::SaveStringToFile(
		FString::Join(ReportLines, LINE_TERMINATOR),
		*ReportPath,
		FFileHelper::EEncodingOptions::ForceUTF8);

	UE_LOG(LogTemp, Display, TEXT("Enemy AI template generator finished. Report: %s"), *ReportPath);
	return bPresetDefaultMelee ? 0 : 1;
}
