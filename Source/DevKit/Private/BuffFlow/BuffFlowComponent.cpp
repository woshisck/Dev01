#include "BuffFlow/BuffFlowComponent.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "Character/YogCharacterBase.h"
#include "Data/RuneDataAsset.h"
#include "Engine/World.h"
#include "FlowSubsystem.h"
#include "FlowAsset.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Nodes/FlowNode.h"

namespace
{
	TAutoConsoleVariable<int32> CVarBuffFlowTrace(
		TEXT("BuffFlow.Trace"),
		0,
		TEXT("Enable compact BuffFlow node execution trace logging and retention."));

	TAutoConsoleVariable<int32> CVarBuffFlowTraceVerbose(
		TEXT("BuffFlow.TraceVerbose"),
		0,
		TEXT("Enable verbose BuffFlow trace log values."));

	const TCHAR* TraceResultToString(EBuffFlowTraceResult Result)
	{
		switch (Result)
		{
		case EBuffFlowTraceResult::Success:
			return TEXT("Success");
		case EBuffFlowTraceResult::Failed:
			return TEXT("Failed");
		case EBuffFlowTraceResult::Skipped:
			return TEXT("Skipped");
		default:
			return TEXT("Unknown");
		}
	}

	void DumpBuffFlowTrace(UWorld* World)
	{
		if (!World)
		{
			return;
		}

		APlayerController* PC = World->GetFirstPlayerController();
		AActor* TraceOwner = PC ? Cast<AActor>(PC->GetPawn()) : nullptr;
		UBuffFlowComponent* BFC = TraceOwner ? TraceOwner->FindComponentByClass<UBuffFlowComponent>() : nullptr;
		if (!BFC)
		{
			UE_LOG(LogTemp, Warning, TEXT("[BuffFlowTrace] Dump failed: no player BuffFlowComponent."));
			return;
		}

		const TArray<FBuffFlowTraceEntry> Entries = BFC->GetTraceEntries();
		UE_LOG(LogTemp, Warning, TEXT("[BuffFlowTrace] Dump Count=%d Owner=%s"), Entries.Num(), *GetNameSafe(TraceOwner));
		for (const FBuffFlowTraceEntry& Entry : Entries)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("[BuffFlowTrace] t=%.2f Result=%s Flow=%s Node=%s Profile=%s Target=%s Card=%s CardId=%s Msg=%s Values=%s"),
				Entry.TimeSeconds,
				TraceResultToString(Entry.Result),
				*Entry.FlowName.ToString(),
				*Entry.NodeName.ToString(),
				*Entry.ProfileName.ToString(),
				*Entry.TargetName.ToString(),
				*Entry.CardName.ToString(),
				*Entry.CardIdTag.ToString(),
				*Entry.Message,
				*Entry.Values);
		}
	}

	FAutoConsoleCommandWithWorld CmdDumpBuffFlowTrace(
		TEXT("Yog_DumpBuffFlowTrace"),
		TEXT("Dump recent BuffFlow trace entries from the current player."),
		FConsoleCommandWithWorldDelegate::CreateStatic(&DumpBuffFlowTrace));
}

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

void UBuffFlowComponent::ClearTraceEntries()
{
	TraceEntries.Reset();
}

bool UBuffFlowComponent::IsTraceEnabled()
{
	return CVarBuffFlowTrace.GetValueOnGameThread() != 0 || CVarBuffFlowTraceVerbose.GetValueOnGameThread() != 0;
}

bool UBuffFlowComponent::IsVerboseTraceEnabled()
{
	return CVarBuffFlowTraceVerbose.GetValueOnGameThread() != 0;
}

void UBuffFlowComponent::RecordTrace(
	UFlowNode* Node,
	UObject* Profile,
	AActor* Target,
	EBuffFlowTraceResult Result,
	const FString& Message,
	const FString& Values)
{
	const bool bTraceEnabled = IsTraceEnabled();
	FBuffFlowTraceEntry Entry;
	Entry.TimeSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	UFlowAsset* FlowAsset = Node ? Node->GetFlowAsset() : nullptr;
	Entry.FlowName = FlowAsset ? FName(*FlowAsset->GetName()) : NAME_None;
	Entry.NodeName = Node ? FName(*Node->GetName()) : NAME_None;
	Entry.NodeClass = Node ? Node->GetClass()->GetFName() : NAME_None;
	Entry.ProfileName = Profile ? FName(*Profile->GetName()) : NAME_None;
	Entry.OwnerName = GetOwner() ? FName(*GetOwner()->GetName()) : NAME_None;
	Entry.TargetName = Target ? FName(*Target->GetName()) : NAME_None;
	if (bHasCombatCardEffectContext)
	{
		const FCombatCardConfig& CardConfig = LastCombatCardEffectContext.SourceCard.Config;
		Entry.CardName = FName(*CardConfig.DisplayName.ToString());
		Entry.CardIdTag = CardConfig.CardIdTag;
	}
	Entry.Result = Result;
	Entry.Message = Message;
	Entry.Values = Values;

	TraceEntries.Add(Entry);
	while (TraceEntries.Num() > FMath::Clamp(MaxTraceEntries, 16, 1000))
	{
		TraceEntries.RemoveAt(0);
	}

	if (bTraceEnabled)
	{
		const bool bVerbose = IsVerboseTraceEnabled();
		UE_LOG(LogTemp, Warning,
			TEXT("[BuffFlowTrace] Result=%s Flow=%s Node=%s Profile=%s Target=%s Card=%s Msg=%s%s%s"),
			TraceResultToString(Result),
			*Entry.FlowName.ToString(),
			*Entry.NodeName.ToString(),
			*Entry.ProfileName.ToString(),
			*Entry.TargetName.ToString(),
			*Entry.CardName.ToString(),
			*Message,
			bVerbose && !Values.IsEmpty() ? TEXT(" Values=") : TEXT(""),
			bVerbose ? *Values : TEXT(""));
	}
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
