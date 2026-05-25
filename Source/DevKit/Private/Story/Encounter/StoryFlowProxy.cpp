#include "Story/Encounter/StoryFlowProxy.h"

#include "FlowAsset.h"
#include "FlowComponent.h"
#include "TimerManager.h"

AStoryFlowProxy::AStoryFlowProxy()
{
	PrimaryActorTick.bCanEverTick = false;
	bNetLoadOnClient = false;
	SetActorHiddenInGame(true);

	FlowComp = CreateDefaultSubobject<UFlowComponent>(TEXT("FlowComp"));
	FlowComp->bAutoStartRootFlow = false;
}

void AStoryFlowProxy::RunFlow(UFlowAsset* FlowAsset)
{
	if (!FlowAsset || !FlowComp)
	{
		return;
	}

	FlowComp->RootFlow = FlowAsset;
	FlowComp->StartRootFlow();

	GetWorldTimerManager().SetTimer(
		PollTimer,
		this,
		&AStoryFlowProxy::PollFlowCompletion,
		0.25f,
		true);
}

void AStoryFlowProxy::StopFlow()
{
	if (FlowComp && FlowComp->RootFlow)
	{
		FlowComp->FinishRootFlow(FlowComp->RootFlow, EFlowFinishPolicy::Keep);
	}

	GetWorldTimerManager().ClearTimer(PollTimer);
}

bool AStoryFlowProxy::IsRunningFlow(const UFlowAsset* FlowAsset) const
{
	return FlowAsset && FlowComp && FlowComp->RootFlow == FlowAsset;
}

void AStoryFlowProxy::PollFlowCompletion()
{
	if (!FlowComp)
	{
		Destroy();
		return;
	}

	if (FlowComp->GetRootInstances(FlowComp).IsEmpty())
	{
		GetWorldTimerManager().ClearTimer(PollTimer);
		Destroy();
	}
}
