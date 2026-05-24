#include "LevelFlow/LevelEventTrigger.h"

#include "Character/PlayerCharacterBase.h"
#include "Components/BoxComponent.h"
#include "FlowComponent.h"
#include "LevelFlow/LevelFlowAsset.h"
#include "Story/Encounter/StoryEncounterGraph.h"
#include "Story/Encounter/StoryEncounterRuntimeSubsystem.h"

ALevelEventTrigger::ALevelEventTrigger()
{
	PrimaryActorTick.bCanEverTick = false;

	TriggerVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerVolume"));
	TriggerVolume->InitBoxExtent(FVector(150.f, 150.f, 100.f));
	TriggerVolume->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	TriggerVolume->SetGenerateOverlapEvents(true);
	TriggerVolume->OnComponentBeginOverlap.AddDynamic(this, &ALevelEventTrigger::OnOverlapBegin);
	TriggerVolume->OnComponentEndOverlap.AddDynamic(this, &ALevelEventTrigger::OnOverlapEnd);
	RootComponent = TriggerVolume;

	LevelFlowComp = CreateDefaultSubobject<UFlowComponent>(TEXT("LevelFlowComp"));
}

void ALevelEventTrigger::BeginPlay()
{
	Super::BeginPlay();
	bTriggered = false;
	UE_LOG(LogTemp, Log, TEXT("[LevelEventTrigger] BeginPlay: %s bTriggerOnce=%d EncounterGraph=%s NodeId=%s"),
		*GetNameSafe(this),
		(int32)bTriggerOnce,
		*GetNameSafe(EncounterGraph),
		*NodeId.ToString());
}

void ALevelEventTrigger::OnOverlapBegin(UPrimitiveComponent*, AActor* OtherActor,
	UPrimitiveComponent*, int32, bool, const FHitResult&)
{
	if (!Cast<APlayerCharacterBase>(OtherActor)) return;
	if (bTriggerOnce && bTriggered) return;

	bool bDidTrigger = false;
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UStoryEncounterRuntimeSubsystem* Runtime = GameInstance->GetSubsystem<UStoryEncounterRuntimeSubsystem>())
		{
			if (EncounterGraph && !NodeId.IsNone())
			{
				bDidTrigger |= Runtime->TriggerEncounterGraphNode(EncounterGraph, NodeId, this);
			}
		}
	}

	if (!bDidTrigger)
	{
		UE_LOG(LogTemp, Warning, TEXT("[LevelEventTrigger] no Story Encounter triggered on %s"), *GetNameSafe(this));
		return;
	}

	bTriggered = true;
}

void ALevelEventTrigger::OnOverlapEnd(UPrimitiveComponent*, AActor* OtherActor,
	UPrimitiveComponent*, int32)
{
	if (!Cast<APlayerCharacterBase>(OtherActor)) return;
	OnPlayerExited.Broadcast();
}

bool ALevelEventTrigger::RunLevelFlow(ULevelFlowAsset* FlowAsset, bool bStopExistingFlow)
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
