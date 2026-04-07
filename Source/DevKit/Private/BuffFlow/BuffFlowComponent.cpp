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

void UBuffFlowComponent::StartBuffFlow(UFlowAsset* FlowAsset, FGuid RuneGuid, AActor* Giver)
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
