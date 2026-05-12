#include "BuffFlow/BuffFlowComponent.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "Character/YogCharacterBase.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "FlowSubsystem.h"
#include "FlowAsset.h"
#include "Nodes/FlowNode.h"

UBuffFlowComponent::UBuffFlowComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UBuffFlowComponent::BeginPlay()
{
	Super::BeginPlay();

	// 缓存 ASC
	if (AActor* Owner = GetOwner())
	{
		CachedASC = Owner->FindComponentByClass<UYogAbilitySystemComponent>();
	}
}

void UBuffFlowComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopAllBuffFlows();
	Super::EndPlay(EndPlayReason);
}

void UBuffFlowComponent::StartBuffFlow(UFlowAsset* FlowAsset, FGuid RuneGuid, AActor* Giver, bool bRestartExistingFlow)
{
	if (!FlowAsset)
	{
		return;
	}

	CurrentBuffGiver = Giver;

	UFlowSubsystem* FlowSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UFlowSubsystem>();
	if (!FlowSubsystem)
	{
		UE_LOG(LogTemp, Warning, TEXT("BuffFlowComponent: FlowSubsystem not found"));
		return;
	}

	TArray<FGuid> ExistingSameFlowGuids;
	for (const TPair<FGuid, TWeakObjectPtr<UFlowAsset>>& Pair : ActiveRuneFlows)
	{
		if (Pair.Value.Get() == FlowAsset)
		{
			ExistingSameFlowGuids.Add(Pair.Key);
		}
	}

	if (!ExistingSameFlowGuids.IsEmpty())
	{
		if (!bRestartExistingFlow)
		{
			UE_LOG(LogTemp, Verbose, TEXT("BuffFlow skipped duplicate Flow=%s Owner=%s Rune=%s"),
				*GetNameSafe(FlowAsset), *GetNameSafe(GetOwner()), *RuneGuid.ToString());
			return;
		}

		FlowSubsystem->FinishRootFlow(this, FlowAsset, EFlowFinishPolicy::Abort);
		for (const FGuid& ExistingGuid : ExistingSameFlowGuids)
		{
			ActiveRuneFlows.Remove(ExistingGuid);
			OnBuffFlowStopped.Broadcast(ExistingGuid);
		}
	}

	FlowSubsystem->StartRootFlow(this, FlowAsset, true);
	ActiveRuneFlows.Add(RuneGuid, FlowAsset);

	OnBuffFlowStarted.Broadcast(RuneGuid);
	UE_LOG(LogTemp, Log, TEXT("BuffFlow started for rune %s"), *RuneGuid.ToString());
}


void UBuffFlowComponent::StopBuffFlow(FGuid RuneGuid)
{
	TWeakObjectPtr<UFlowAsset>* FoundAsset = ActiveRuneFlows.Find(RuneGuid);
	if (!FoundAsset || !FoundAsset->IsValid())
	{
		ActiveRuneFlows.Remove(RuneGuid);
		return;
	}

	UFlowAsset* FlowAsset = FoundAsset->Get();

	UFlowSubsystem* FlowSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UFlowSubsystem>();
	if (FlowSubsystem)
	{
		FlowSubsystem->FinishRootFlow(this, FlowAsset, EFlowFinishPolicy::Abort);
	}

	ActiveRuneFlows.Remove(RuneGuid);
	OnBuffFlowStopped.Broadcast(RuneGuid);
	UE_LOG(LogTemp, Log, TEXT("BuffFlow stopped for rune %s"), *RuneGuid.ToString());
}

void UBuffFlowComponent::StopAllBuffFlows()
{
	UFlowSubsystem* FlowSubsystem = nullptr;
	if (GetWorld() && GetWorld()->GetGameInstance())
	{
		FlowSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UFlowSubsystem>();
	}

	if (FlowSubsystem)
	{
		for (auto& Pair : ActiveRuneFlows)
		{
			if (Pair.Value.IsValid())
			{
				FlowSubsystem->FinishRootFlow(this, Pair.Value.Get(), EFlowFinishPolicy::Abort);
			}
		}
	}

	ActiveRuneFlows.Empty();
}

UYogAbilitySystemComponent* UBuffFlowComponent::GetASC() const
{
	return CachedASC.Get();
}

AYogCharacterBase* UBuffFlowComponent::GetBuffOwner() const
{
	return Cast<AYogCharacterBase>(GetOwner());
}

AActor* UBuffFlowComponent::GetBuffGiver() const
{
	return CurrentBuffGiver.Get();
}

void UBuffFlowComponent::ClearTraceEntries()
{
	TraceEntries.Reset();
}

int32 UBuffFlowComponent::GetActiveBuffFlowCount() const
{
	return ActiveRuneFlows.Num();
}

TArray<FBuffFlowActiveFlowDebugEntry> UBuffFlowComponent::GetActiveBuffFlowDebugEntries() const
{
	TArray<FBuffFlowActiveFlowDebugEntry> Entries;
	Entries.Reserve(ActiveRuneFlows.Num());

	UFlowSubsystem* FlowSubsystem = nullptr;
	if (UWorld* World = GetWorld())
	{
		if (UGameInstance* GameInstance = World->GetGameInstance())
		{
			FlowSubsystem = GameInstance->GetSubsystem<UFlowSubsystem>();
		}
	}

	for (const TPair<FGuid, TWeakObjectPtr<UFlowAsset>>& Pair : ActiveRuneFlows)
	{
		FBuffFlowActiveFlowDebugEntry& Entry = Entries.AddDefaulted_GetRef();
		Entry.RuneGuid = Pair.Key;

		UFlowAsset* FlowAsset = Pair.Value.Get();
		Entry.bFlowAssetValid = FlowAsset != nullptr;
		Entry.FlowName = FlowAsset ? FName(*FlowAsset->GetName()) : NAME_None;

		if (FlowAsset && FlowSubsystem)
		{
			for (UFlowAsset* RuntimeInstance : FlowSubsystem->GetRootInstancesByOwner(this))
			{
				if (RuntimeInstance && RuntimeInstance->GetTemplateAsset() == FlowAsset)
				{
					Entry.bRuntimeInstanceActive = RuntimeInstance->IsActive();

					const TArray<UFlowNode*>& ActiveNodes = RuntimeInstance->GetActiveNodes();
					Entry.ActiveNodeCount = ActiveNodes.Num();
					Entry.ActiveNodeNames.Reserve(ActiveNodes.Num());
					Entry.ActiveNodeClasses.Reserve(ActiveNodes.Num());
					for (const UFlowNode* ActiveNode : ActiveNodes)
					{
						Entry.ActiveNodeNames.Add(ActiveNode ? FName(*ActiveNode->GetName()) : NAME_None);
						Entry.ActiveNodeClasses.Add(ActiveNode ? ActiveNode->GetClass()->GetFName() : NAME_None);
					}

					const TArray<UFlowNode*>& RecordedNodes = RuntimeInstance->GetRecordedNodes();
					Entry.RecordedNodeCount = RecordedNodes.Num();
					Entry.RecordedNodeNames.Reserve(RecordedNodes.Num());
					for (const UFlowNode* RecordedNode : RecordedNodes)
					{
						Entry.RecordedNodeNames.Add(RecordedNode ? FName(*RecordedNode->GetName()) : NAME_None);
					}

					break;
				}
			}
		}
	}

	return Entries;
}
