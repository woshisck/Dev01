#include "BuffFlow/BuffFlowComponent.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "Character/YogCharacterBase.h"
#include "FlowSubsystem.h"
#include "FlowAsset.h"

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
	bHasCombatCardEffectContext = false;
	LastCombatCardEffectContext = FCombatCardEffectContext();
	StartBuffFlowInternal(FlowAsset, RuneGuid, Giver, bRestartExistingFlow);
}

void UBuffFlowComponent::StartCombatCardFlow(
	UFlowAsset* FlowAsset,
	const FCombatCardInstance& Card,
	const FCombatDeckActionContext& ActionContext,
	const FCombatCardResolveResult& ResolveResult,
	AActor* Giver,
	bool bRestartExistingFlow)
{
	LastCombatCardEffectContext.ActionContext = ActionContext;
	LastCombatCardEffectContext.SourceCard = Card;
	LastCombatCardEffectContext.ResolveResult = ResolveResult;
	LastCombatCardEffectContext.ComboIndex = ActionContext.ComboIndex;
	LastCombatCardEffectContext.ComboNodeId = ActionContext.ComboNodeId;
	LastCombatCardEffectContext.ComboTags = ActionContext.ComboTags;
	LastCombatCardEffectContext.AbilityTag = ActionContext.AbilityTag;
	LastCombatCardEffectContext.EffectMultiplier = ResolveResult.AppliedMultiplier;
	LastCombatCardEffectContext.ComboBonusStacks = FMath::Max(0, ActionContext.ComboIndex - 1);
	LastCombatCardEffectContext.bFromLink = ResolveResult.bTriggeredLink || ResolveResult.bTriggeredForwardLink || ResolveResult.bTriggeredBackwardLink;
	LastCombatCardEffectContext.bIsComboFinisher = ActionContext.bIsComboFinisher;
	bHasCombatCardEffectContext = true;

	StartBuffFlowInternal(FlowAsset, Card.InstanceGuid, Giver, bRestartExistingFlow);
}

void UBuffFlowComponent::StartBuffFlowInternal(UFlowAsset* FlowAsset, FGuid RuneGuid, AActor* Giver, bool bRestartExistingFlow)
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

UFlowAsset* UBuffFlowComponent::GetActiveBuffFlowAsset(FGuid RuneGuid) const
{
	const TWeakObjectPtr<UFlowAsset>* FoundAsset = ActiveRuneFlows.Find(RuneGuid);
	return FoundAsset ? FoundAsset->Get() : nullptr;
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
