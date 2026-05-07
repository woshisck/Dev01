#include "DevKitEditor/EnemyAI/EnemyAITemplateGeneratorCommandlet.h"

#include "AI/BTDecorator_EnemyAIState.h"
#include "AI/BTService_UpdateEnemyAwareness.h"
#include "Commandlets/CommandletReportUtils.h"
#include "AI/BTService_UpdateEnemyCombatMove.h"
#include "AI/BTTask_EnemyCombatMove.h"
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
#include "Data/AbilityData.h"
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

	enum class EDefaultEnemyProfile : uint8
	{
		Rat,
		RottenGuard,
	};

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

	FEnemyAIAttackOption MakeAttackOption(
		const TCHAR* AttackName,
		const TCHAR* TagName,
		float MinRange,
		float MaxRange,
		float Weight,
		float Cooldown,
		EEnemyAIAttackRole AttackRole = EEnemyAIAttackRole::CloseMelee,
		EEnemyAIAttackMovementMode MovementMode = EEnemyAIAttackMovementMode::None,
		float LungeStartRange = 0.0f,
		float LungeDistance = 0.0f,
		float LungeDuration = 0.35f,
		float LungeStopDistance = 0.0f,
		float MovementAttackRangeMultiplier = 2.5f,
		float MovementAttackCooldown = 10.0f)
	{
		FEnemyAIAttackOption Option;
		Option.AttackName = FName(AttackName);
		Option.MinRange = MinRange;
		Option.MaxRange = MaxRange;
		Option.Weight = Weight;
		Option.Cooldown = Cooldown;
		Option.bPreAttackFlash = true;
		Option.AttackRole = AttackRole;
		Option.AttackMovementMode = MovementMode;
		Option.LungeStartRange = LungeStartRange;
		Option.LungeDistance = LungeDistance;
		Option.LungeDuration = LungeDuration;
		Option.LungeStopDistance = LungeStopDistance;
		Option.MovementAttackRangeMultiplier = MovementAttackRangeMultiplier;
		Option.MovementAttackCooldown = MovementAttackCooldown;

		const FGameplayTag AbilityTag = FGameplayTag::RequestGameplayTag(FName(TagName), false);
		if (AbilityTag.IsValid())
		{
			Option.AbilityTags.AddTag(AbilityTag);
		}

		return Option;
	}

	void UpsertAttackOption(FEnemyAIAttackProfile& AttackProfile, const FEnemyAIAttackOption& Option)
	{
		for (FEnemyAIAttackOption& Existing : AttackProfile.Attacks)
		{
			if (Existing.AttackName == Option.AttackName)
			{
				Existing = Option;
				return;
			}
		}

		AttackProfile.Attacks.Add(Option);
	}

	bool AttackOptionHasAbilityTag(const FEnemyAIAttackOption& Option, const FGameplayTag& AbilityTag)
	{
		return AbilityTag.IsValid() && Option.AbilityTags.HasTagExact(AbilityTag);
	}

	bool AttackProfileHasAbilityTag(const FEnemyAIAttackProfile& AttackProfile, const FGameplayTag& AbilityTag)
	{
		for (const FEnemyAIAttackOption& Attack : AttackProfile.Attacks)
		{
			if (AttackOptionHasAbilityTag(Attack, AbilityTag))
			{
				return true;
			}
		}
		return false;
	}

	void UpsertAttackOptionByTag(FEnemyAIAttackProfile& AttackProfile, const FEnemyAIAttackOption& Option)
	{
		for (const FGameplayTag& AbilityTag : Option.AbilityTags)
		{
			if (!AbilityTag.IsValid())
			{
				continue;
			}

			for (FEnemyAIAttackOption& Existing : AttackProfile.Attacks)
			{
				if (AttackOptionHasAbilityTag(Existing, AbilityTag))
				{
					Existing = Option;
					return;
				}
			}
		}

		UpsertAttackOption(AttackProfile, Option);
	}

	FString GetAttackNameFromTag(const FGameplayTag& AbilityTag)
	{
		const FString TagString = AbilityTag.ToString();
		int32 LastDotIndex = INDEX_NONE;
		return TagString.FindLastChar(TEXT('.'), LastDotIndex) ? TagString.RightChop(LastDotIndex + 1) : TagString;
	}

	bool IsGeneratedAttackTag(const FString& TagString)
	{
		return TagString.StartsWith(TEXT("Enemy.Melee.")) || TagString.StartsWith(TEXT("Enemy.Skill."));
	}

	FEnemyAIAttackOption MakeGeneratedAttackOptionFromAbilityTag(const UEnemyData& EnemyData, const FGameplayTag& AbilityTag)
	{
		const FString TagString = AbilityTag.ToString();
		const FString AttackName = GetAttackNameFromTag(AbilityTag);
		const float CloseAttackRange = FMath::Max(EnemyData.MovementTuning.AttackRange, 1.0f);

		if (TagString.StartsWith(TEXT("Enemy.Skill.")))
		{
			const float SkillRange = FMath::Max(EnemyData.AwarenessTuning.CombatEnterRadius, CloseAttackRange);
			return MakeAttackOption(
				*AttackName,
				*TagString,
				0.0f,
				SkillRange,
				1.0f,
				15.0f,
				EEnemyAIAttackRole::Skill);
		}

		if (TagString.Contains(TEXT(".LAtk")))
		{
			return MakeAttackOption(
				*AttackName,
				*TagString,
				0.0f,
				CloseAttackRange,
				2.5f,
				0.9f,
				EEnemyAIAttackRole::CloseMelee);
		}

		const float HeavyCloseRange = FMath::Max(CloseAttackRange + EnemyData.MovementTuning.AttackRangeExitBuffer, CloseAttackRange);
		return MakeAttackOption(
			*AttackName,
			*TagString,
			0.0f,
			HeavyCloseRange,
			1.25f,
			1.4f,
			EEnemyAIAttackRole::CloseMelee);
	}

	void SyncAttackProfileFromAbilityData(UEnemyData& EnemyData)
	{
		const UAbilityData* AbilityData = EnemyData.AbilityData;
		if (!AbilityData)
		{
			return;
		}

		TArray<FGameplayTag> ValidAttackTags;
		auto AddValidAttackTag = [&ValidAttackTags, AbilityData](const FGameplayTag& AbilityTag)
		{
			const FString TagString = AbilityTag.ToString();
			if (!AbilityTag.IsValid() || !IsGeneratedAttackTag(TagString) || !AbilityData->HasAbility(AbilityTag))
			{
				return;
			}
			ValidAttackTags.AddUnique(AbilityTag);
		};

		for (const TPair<FGameplayTag, TObjectPtr<UAnimMontage>>& MontageEntry : AbilityData->MontageMap)
		{
			AddValidAttackTag(MontageEntry.Key);
		}
		for (const TPair<FGameplayTag, FAbilityMontageConfigList>& ConfigEntry : AbilityData->MontageConfigMap)
		{
			AddValidAttackTag(ConfigEntry.Key);
		}

		ValidAttackTags.Sort([](const FGameplayTag& A, const FGameplayTag& B)
		{
			return A.ToString() < B.ToString();
		});

		for (const FGameplayTag& AbilityTag : ValidAttackTags)
		{
			if (AttackProfileHasAbilityTag(EnemyData.AttackProfile, AbilityTag))
			{
				continue;
			}

			EnemyData.AttackProfile.Attacks.Add(MakeGeneratedAttackOptionFromAbilityTag(EnemyData, AbilityTag));
		}
	}

	void ConfigureDefaultEnemyData(UEnemyData& EnemyData, EDefaultEnemyProfile Profile)
	{
		FEnemyAIAwarenessTuning Awareness;
		Awareness.DetectionRadius = 900.f;
		Awareness.CombatEnterRadius = 650.f;
		Awareness.CombatExitRadius = 1200.f;
		Awareness.LoseTargetDelay = 2.0f;
		Awareness.AlertDuration = 4.0f;
		Awareness.AlertBroadcastRadius = 1200.f;
		Awareness.PatrolRadius = 600.f;
		Awareness.PatrolWaitMin = 0.6f;
		Awareness.PatrolWaitMax = 1.5f;
		EnemyData.AwarenessTuning = Awareness;
		EnemyData.AttackProfile.RecentAttackMemoryDuration = 2.0f;
		EnemyData.AttackProfile.RepeatAttackWeightMultiplier = 0.25f;

		switch (Profile)
		{
		case EDefaultEnemyProfile::Rat:
			EnemyData.MovementTuning.ApproachStyle = EEnemyAIApproachStyle::SwarmFlank;
			EnemyData.MovementTuning.PreferredRange = 180.f;
			EnemyData.MovementTuning.AttackRange = 150.f;
			EnemyData.MovementTuning.AcceptanceRadius = 60.f;
			EnemyData.MovementTuning.RepathInterval = 0.2f;
			EnemyData.MovementTuning.FlankDistance = 160.f;
			EnemyData.MovementTuning.StrafeChance = 0.45f;
			EnemyData.MovementTuning.CrowdSeparationWeight = 2.4f;
			EnemyData.MovementTuning.bUseForwardSteering = true;
			EnemyData.MovementTuning.ForwardTurnLeadDistance = 170.f;
			EnemyData.MovementTuning.MaxTurnYawSpeed = 520.f;
			EnemyData.MovementTuning.MoveTargetSmoothingSpeed = 8.0f;
			EnemyData.MovementTuning.SharpTurnAngle = 125.f;
			EnemyData.MovementTuning.MaxWalkSpeedOverride = 0.f;
			EnemyData.MovementTuning.CombatSlotLockDuration = 1.2f;
			EnemyData.MovementTuning.AttackRangeExitBuffer = 40.f;
			UpsertAttackOptionByTag(EnemyData.AttackProfile, MakeAttackOption(TEXT("QuickBite"), TEXT("Enemy.Melee.LAtk1"), 0.f, 170.f, 2.0f, 0.8f));
			UpsertAttackOptionByTag(EnemyData.AttackProfile, MakeAttackOption(TEXT("Bite"), TEXT("Enemy.Melee.LAtk2"), 0.f, 180.f, 1.0f, 1.1f));
			break;

		case EDefaultEnemyProfile::RottenGuard:
			EnemyData.MovementTuning.ApproachStyle = EEnemyAIApproachStyle::BruiserHold;
			EnemyData.MovementTuning.PreferredRange = 320.f;
			EnemyData.MovementTuning.AttackRange = 260.f;
			EnemyData.MovementTuning.AcceptanceRadius = 110.f;
			EnemyData.MovementTuning.RepathInterval = 0.35f;
			EnemyData.MovementTuning.FlankDistance = 120.f;
			EnemyData.MovementTuning.StrafeChance = 0.15f;
			EnemyData.MovementTuning.CrowdSeparationWeight = 3.0f;
			EnemyData.MovementTuning.bUseForwardSteering = true;
			EnemyData.MovementTuning.ForwardTurnLeadDistance = 240.f;
			EnemyData.MovementTuning.MaxTurnYawSpeed = 260.f;
			EnemyData.MovementTuning.MoveTargetSmoothingSpeed = 5.0f;
			EnemyData.MovementTuning.SharpTurnAngle = 105.f;
			EnemyData.MovementTuning.MaxWalkSpeedOverride = 420.f;
			EnemyData.MovementTuning.CombatSlotLockDuration = 0.4f;
			EnemyData.MovementTuning.AttackRangeExitBuffer = 40.f;
			UpsertAttackOptionByTag(EnemyData.AttackProfile, MakeAttackOption(TEXT("Sweep"), TEXT("Enemy.Melee.HAtk1"), 0.f, 290.f, 2.0f, 1.6f));
			UpsertAttackOptionByTag(EnemyData.AttackProfile, MakeAttackOption(
				TEXT("Heavy"),
				TEXT("Enemy.Melee.HAtk2"),
				160.f,
				650.f,
				1.0f,
				2.2f,
				EEnemyAIAttackRole::SpecialMovement,
				EEnemyAIAttackMovementMode::RadialLunge,
				300.f,
				280.f,
				0.35f,
				170.f,
				2.5f,
				10.0f));
			break;
		}

		SyncAttackProfileFromAbilityData(EnemyData);
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

		UBehaviorTreeGraphNode_Task* SkillTask = AddNode<UBehaviorTreeGraphNode_Task, UBTTask_EnemyAttackByProfile>(BehaviorTree, *Graph, -850, 560);
		if (UBTTask_EnemyAttackByProfile* AttackTask = Cast<UBTTask_EnemyAttackByProfile>(SkillTask->NodeInstance))
		{
			AttackTask->SetRequiredAttackRole(EEnemyAIAttackRole::Skill);
		}
		Connect(CombatSelector, SkillTask);

		UBehaviorTreeGraphNode_Task* SpecialMovementTask = AddNode<UBehaviorTreeGraphNode_Task, UBTTask_EnemyAttackByProfile>(BehaviorTree, *Graph, -650, 560);
		if (UBTTask_EnemyAttackByProfile* AttackTask = Cast<UBTTask_EnemyAttackByProfile>(SpecialMovementTask->NodeInstance))
		{
			AttackTask->SetRequiredAttackRole(EEnemyAIAttackRole::SpecialMovement);
		}
		Connect(CombatSelector, SpecialMovementTask);

		UBehaviorTreeGraphNode_Task* CloseMeleeTask = AddNode<UBehaviorTreeGraphNode_Task, UBTTask_EnemyAttackByProfile>(BehaviorTree, *Graph, -450, 560);
		if (UBTTask_EnemyAttackByProfile* AttackTask = Cast<UBTTask_EnemyAttackByProfile>(CloseMeleeTask->NodeInstance))
		{
			AttackTask->SetRequiredAttackRole(EEnemyAIAttackRole::CloseMelee);
		}
		Connect(CombatSelector, CloseMeleeTask);

		UBehaviorTreeGraphNode_Task* CombatMoveTask = AddNode<UBehaviorTreeGraphNode_Task, UBTTask_EnemyCombatMove>(BehaviorTree, *Graph, -250, 560);
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

	void AssignBehaviorTreeToEnemyData(
		const FString& EnemyDataPath,
		UBehaviorTree* BehaviorTree,
		EDefaultEnemyProfile Profile,
		bool bDryRun,
		TArray<FString>& ReportLines,
		TArray<UPackage*>& DirtyPackages)
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
		ConfigureDefaultEnemyData(*EnemyData, Profile);
		EnemyData->MarkPackageDirty();
		DirtyPackages.AddUnique(EnemyData->GetPackage());
		ReportLines.Add(FString::Printf(TEXT("- Ensured `%s` default movement, awareness and attack profile."), *EnemyDataPath));
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
		ReportLines.Add(TEXT("- Combat branch order: Skill -> SpecialMovement -> CloseMelee -> MoveToCombatSlot."));
		UBehaviorTree* BehaviorTree = CreateOrLoadAsset<UBehaviorTree>(BehaviorTreePath, bDryRun, ReportLines, DirtyPackages);
		if (!bDryRun && BehaviorTree && Blackboard)
		{
			RebuildBehaviorTreeGraph(*BehaviorTree, *Blackboard);
			DirtyPackages.AddUnique(BehaviorTree->GetPackage());
			ReportLines.Add(TEXT("- Rebuilt visual graph and runtime tree."));
		}

		ReportLines.Add(TEXT(""));
		ReportLines.Add(TEXT("## Enemy Data"));
		AssignBehaviorTreeToEnemyData(RatDataPath, BehaviorTree, EDefaultEnemyProfile::Rat, bDryRun, ReportLines, DirtyPackages);
		AssignBehaviorTreeToEnemyData(RottenGuardDataPath, BehaviorTree, EDefaultEnemyProfile::RottenGuard, bDryRun, ReportLines, DirtyPackages);
	}

	if (!bDryRun && DirtyPackages.Num() > 0)
	{
		UEditorLoadingAndSavingUtils::SavePackages(DirtyPackages, false);
	}

	FString ReportPath;
	FString SharedReportPath;
	DevKitEditorCommandletReports::SaveReportLines(TEXT("EnemyAITemplateGeneratorReport.md"), ReportLines, ReportPath, SharedReportPath);

	UE_LOG(LogTemp, Display, TEXT("Enemy AI template generator finished. Report: %s Shared: %s"), *ReportPath, *SharedReportPath);
	return bPresetDefaultMelee ? 0 : 1;
}
