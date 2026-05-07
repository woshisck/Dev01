#include "BuffFlow/BuffFlowComponent.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "Character/YogCharacterBase.h"
#include "Data/RuneDataAsset.h"
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

void UBuffFlowComponent::StartBuffFlowWithRune(UFlowAsset* FlowAsset, FGuid RuneGuid, URuneDataAsset* SourceRune, AActor* Giver, bool bRestartExistingFlow)
{
	bHasCombatCardEffectContext = false;
	LastCombatCardEffectContext = FCombatCardEffectContext();
	StartBuffFlowInternal(FlowAsset, RuneGuid, Giver, bRestartExistingFlow, false, SourceRune);
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

void UBuffFlowComponent::StartCombatCardFlowWithSourceTransform(
	UFlowAsset* FlowAsset,
	const FCombatCardInstance& Card,
	const FCombatDeckActionContext& ActionContext,
	const FCombatCardResolveResult& ResolveResult,
	AActor* Giver,
	const FTransform& SourceTransform,
	bool bRestartExistingFlow)
{
	bHasSourceTransformOverride = true;
	SourceTransformOverride = SourceTransform;
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
	StartBuffFlowInternal(FlowAsset, FGuid::NewGuid(), Giver, bRestartExistingFlow, true);
	bHasSourceTransformOverride = false;
	SourceTransformOverride = FTransform::Identity;
}

bool UBuffFlowComponent::GetActiveSourceTransformOverride(FTransform& OutTransform) const
{
	if (!bHasSourceTransformOverride)
	{
		return false;
	}

	OutTransform = SourceTransformOverride;
	return true;
}

void UBuffFlowComponent::StartBuffFlowInternal(UFlowAsset* FlowAsset, FGuid RuneGuid, AActor* Giver, bool bRestartExistingFlow, bool bAllowParallelSameFlow, URuneDataAsset* SourceRune)
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

	if (!bAllowParallelSameFlow && !ExistingSameFlowGuids.IsEmpty())
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
			ActiveRuneSources.Remove(ExistingGuid);
			OnBuffFlowStopped.Broadcast(ExistingGuid);
		}
	}

	FlowSubsystem->StartRootFlow(this, FlowAsset, true);
	ActiveRuneFlows.Add(RuneGuid, FlowAsset);
	if (SourceRune)
	{
		ActiveRuneSources.Add(RuneGuid, SourceRune);
	}
	else
	{
		ActiveRuneSources.Remove(RuneGuid);
	}

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
	ActiveRuneSources.Remove(RuneGuid);
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
	ActiveRuneSources.Empty();
}

UFlowAsset* UBuffFlowComponent::GetActiveBuffFlowAsset(FGuid RuneGuid) const
{
	const TWeakObjectPtr<UFlowAsset>* FoundAsset = ActiveRuneFlows.Find(RuneGuid);
	return FoundAsset ? FoundAsset->Get() : nullptr;
}

URuneDataAsset* UBuffFlowComponent::GetActiveSourceRuneData(UFlowAsset* FlowAsset) const
{
	if (!FlowAsset)
	{
		return nullptr;
	}

	for (const TPair<FGuid, TWeakObjectPtr<UFlowAsset>>& Pair : ActiveRuneFlows)
	{
		if (Pair.Value.Get() == FlowAsset)
		{
			if (const TWeakObjectPtr<URuneDataAsset>* FoundRune = ActiveRuneSources.Find(Pair.Key))
			{
				return FoundRune->Get();
			}
		}
	}

	return nullptr;
}

float UBuffFlowComponent::GetRuneTuningValueForFlow(UFlowAsset* FlowAsset, FName Key, float DefaultValue) const
{
	URuneDataAsset* Rune = GetActiveSourceRuneData(FlowAsset);
	return Rune ? Rune->GetRuneTuningValue(Key, DefaultValue) : DefaultValue;
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
