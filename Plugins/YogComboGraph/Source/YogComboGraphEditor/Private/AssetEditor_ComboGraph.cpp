#include "AssetEditor_ComboGraph.h"

#include "Data/GameplayAbilityComboGraph.h"
#include "Editor.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "GenericGraphNode.h"
#include "Interfaces/YogComboGraphActiveInstance.h"

FAssetEditor_ComboGraph::FAssetEditor_ComboGraph()
{
	PIEStartHandle = FEditorDelegates::PostPIEStarted.AddRaw(this, &FAssetEditor_ComboGraph::OnPIEStarted);
	PIEEndHandle   = FEditorDelegates::PrePIEEnded.AddRaw(this,   &FAssetEditor_ComboGraph::OnPIEEnded);
}

FAssetEditor_ComboGraph::~FAssetEditor_ComboGraph()
{
	FEditorDelegates::PostPIEStarted.Remove(PIEStartHandle);
	FEditorDelegates::PrePIEEnded.Remove(PIEEndHandle);

	if (DebugTickHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(DebugTickHandle);
	}

	ClearDebugState();
}

void FAssetEditor_ComboGraph::OnPIEStarted(bool /*bIsSimulating*/)
{
	DebugTickHandle = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateRaw(this, &FAssetEditor_ComboGraph::TickDebugger));
}

void FAssetEditor_ComboGraph::OnPIEEnded(bool /*bIsSimulating*/)
{
	if (DebugTickHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(DebugTickHandle);
		DebugTickHandle.Reset();
	}
	ClearDebugState();
}

bool FAssetEditor_ComboGraph::TickDebugger(float /*DeltaTime*/)
{
	UGameplayAbilityComboGraph* ComboGraph = Cast<UGameplayAbilityComboGraph>(EditingGraph);
	if (!ComboGraph)
	{
		return true;
	}

	// Clear all debug highlights first.
	for (UGenericGraphNode* Node : ComboGraph->AllNodes)
	{
		if (UGameplayAbilityComboGraphNode* ComboNode = Cast<UGameplayAbilityComboGraphNode>(Node))
		{
			ComboNode->bDebugActive = false;
		}
	}

	if (!GEditor || !GEditor->PlayWorld)
	{
		return true;
	}

	// Find any object in the PIE world that implements IYogComboGraphActiveInstance
	// and is currently running this specific ComboGraph asset.
	FName ActiveNodeId = NAME_None;
	for (TActorIterator<AActor> ActorIt(GEditor->PlayWorld); ActorIt; ++ActorIt)
	{
		AActor* Actor = *ActorIt;
		if (!Actor)
		{
			continue;
		}

		for (UActorComponent* Component : Actor->GetComponents())
		{
			IYogComboGraphActiveInstance* Active = Cast<IYogComboGraphActiveInstance>(Component);
			if (!Active || Active->GetActiveComboGraph() != ComboGraph)
			{
				continue;
			}
			const FName CandidateId = Active->GetActiveComboNodeId();
			if (!CandidateId.IsNone())
			{
				ActiveNodeId = CandidateId;
				break;
			}
		}

		if (!ActiveNodeId.IsNone())
		{
			break;
		}
	}

	if (ActiveNodeId.IsNone())
	{
		return true;
	}

	for (UGenericGraphNode* Node : ComboGraph->AllNodes)
	{
		UGameplayAbilityComboGraphNode* ComboNode = Cast<UGameplayAbilityComboGraphNode>(Node);
		if (!ComboNode)
		{
			continue;
		}
		// Match by NodeId; fall back to object name (mirrors GetRuntimeNodeId in .cpp).
		const FName RuntimeId = ComboNode->NodeId.IsNone() ? FName(*ComboNode->GetName()) : ComboNode->NodeId;
		if (RuntimeId == ActiveNodeId)
		{
			ComboNode->bDebugActive = true;
			break;
		}
	}

	return true;
}

void FAssetEditor_ComboGraph::ClearDebugState()
{
	if (!EditingGraph)
	{
		return;
	}
	for (UGenericGraphNode* Node : EditingGraph->AllNodes)
	{
		if (UGameplayAbilityComboGraphNode* ComboNode = Cast<UGameplayAbilityComboGraphNode>(Node))
		{
			ComboNode->bDebugActive = false;
		}
	}
}
