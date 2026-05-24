#include "Story/Encounter/StoryEncounterTrigger.h"

#include "Character/PlayerCharacterBase.h"
#include "Components/BoxComponent.h"
#include "FlowComponent.h"
#include "LevelFlow/LevelFlowAsset.h"
#include "Story/Encounter/StoryEncounterGraph.h"
#include "Story/Encounter/StoryEncounterGraphNode.h"
#include "Story/Encounter/StoryEncounterMap.h"
#include "Story/Encounter/StoryEncounterPointDataAsset.h"
#include "Story/Encounter/StoryEncounterRuntimeSubsystem.h"

AStoryEncounterTrigger::AStoryEncounterTrigger()
{
	PrimaryActorTick.bCanEverTick = false;

	TriggerVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerVolume"));
	SetRootComponent(TriggerVolume);
	TriggerVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TriggerVolume->SetCollisionObjectType(ECC_WorldDynamic);
	TriggerVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
	TriggerVolume->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	TriggerVolume->OnComponentBeginOverlap.AddDynamic(this, &AStoryEncounterTrigger::OnTriggerBeginOverlap);
	TriggerVolume->OnComponentEndOverlap.AddDynamic(this, &AStoryEncounterTrigger::OnTriggerEndOverlap);

	LevelFlowComp = CreateDefaultSubobject<UFlowComponent>(TEXT("LevelFlowComp"));
}

void AStoryEncounterTrigger::OnTriggerBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!Cast<APlayerCharacterBase>(OtherActor) || ShouldBlockRepeatTrigger())
	{
		return;
	}

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UStoryEncounterRuntimeSubsystem* Runtime = GameInstance->GetSubsystem<UStoryEncounterRuntimeSubsystem>())
		{
			bool bTriggeredNow = false;
			if (EncounterPoint)
			{
				bTriggeredNow = Runtime->TriggerEncounterPoint(EncounterPoint, this);
			}
			else if (EncounterGraph && !NodeId.IsNone())
			{
				bTriggeredNow = Runtime->TriggerEncounterGraphNode(EncounterGraph, NodeId, this);
			}
			else if (EncounterMap && !NodeId.IsNone())
			{
				bTriggeredNow = Runtime->TriggerEncounterNode(EncounterMap, NodeId, this);
			}

			if (bTriggeredNow)
			{
				bTriggered = true;
			}
		}
	}
}

void AStoryEncounterTrigger::OnTriggerEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (!Cast<APlayerCharacterBase>(OtherActor))
	{
		return;
	}

	OnPlayerExited.Broadcast();
}

bool AStoryEncounterTrigger::RunLevelFlow(ULevelFlowAsset* FlowAsset, bool bStopExistingFlow)
{
	if (!FlowAsset || !LevelFlowComp)
	{
		return false;
	}

	if (bStopExistingFlow)
	{
		LevelFlowComp->FinishRootFlow(LevelFlowComp->RootFlow, EFlowFinishPolicy::Keep);
	}
	LevelFlowComp->RootFlow = FlowAsset;
	LevelFlowComp->StartRootFlow();
	return true;
}

bool AStoryEncounterTrigger::ShouldBlockRepeatTrigger() const
{
	EStoryEncounterFirePolicy FirePolicy = EStoryEncounterFirePolicy::Once;
	if (EncounterPoint)
	{
		FirePolicy = EncounterPoint->FirePolicy;
	}
	else if (EncounterGraph && !NodeId.IsNone())
	{
		if (const UStoryEncounterGraphNode* GraphNode = EncounterGraph->FindNodeByStoryNodeId(NodeId))
		{
			if (const UStoryEncounterPointDA* Point = GraphNode->GetPoint())
			{
				FirePolicy = Point->FirePolicy;
			}
			else
			{
				FirePolicy = GraphNode->ToEncounterNode().FirePolicy;
			}
		}
	}
	else if (EncounterMap && !NodeId.IsNone())
	{
		if (const FStoryEncounterNode* Node = EncounterMap->FindNode(NodeId))
		{
			FirePolicy = Node->FirePolicy;
		}
	}

	return FirePolicy != EStoryEncounterFirePolicy::Repeat && bTriggered;
}
