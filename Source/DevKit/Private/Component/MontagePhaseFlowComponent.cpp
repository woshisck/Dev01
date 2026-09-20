#include "Component/MontagePhaseFlowComponent.h"

#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "BuffFlow/BuffFlowComponent.h"
#include "FlowAsset.h"

namespace
{
	const FGameplayTag& MontagePhaseFlow_PhaseRootTag()
	{
		static const FGameplayTag TAG_PhaseRoot = FGameplayTag::RequestGameplayTag(TEXT("Character.State.Phase"), false);
		return TAG_PhaseRoot;
	}
}

UMontagePhaseFlowComponent::UMontagePhaseFlowComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UMontagePhaseFlowComponent::BeginPlay()
{
	Super::BeginPlay();

	if (UYogAbilitySystemComponent* ASC = GetOwnerASC())
	{
		BoundASC = ASC;
		ASC->OnGameplayTagChanged.AddDynamic(this, &UMontagePhaseFlowComponent::HandleGameplayTagChanged);
	}
}

void UMontagePhaseFlowComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (BoundASC.IsValid())
	{
		BoundASC->OnGameplayTagChanged.RemoveDynamic(this, &UMontagePhaseFlowComponent::HandleGameplayTagChanged);
		BoundASC.Reset();
	}

	Bindings.Reset();

	Super::EndPlay(EndPlayReason);
}

FGuid UMontagePhaseFlowComponent::BindPhaseFlow(FGameplayTag Phase, UFlowAsset* Flow, EBFTargetSelector Target)
{
	if (!Phase.IsValid() || !Flow)
	{
		UE_LOG(LogTemp, Warning, TEXT("[PhaseFlow] BindPhaseFlow rejected on %s: Phase=%s Flow=%s."),
			*GetNameSafe(GetOwner()), *Phase.ToString(), *GetNameSafe(Flow));
		return FGuid();
	}

	if (!Phase.MatchesTag(MontagePhaseFlow_PhaseRootTag()))
	{
		UE_LOG(LogTemp, Warning, TEXT("[PhaseFlow] BindPhaseFlow rejected on %s: %s is not under Character.State.Phase, so no montage would ever launch it."),
			*GetNameSafe(GetOwner()), *Phase.ToString());
		return FGuid();
	}

	// Duplicates would each launch the flow again on the same phase, multiplying the effect by the
	// number of redundant calls. Hand back the existing handle instead so re-binding is a no-op.
	for (const TPair<FGuid, FMontagePhaseFlowBinding>& Pair : Bindings)
	{
		const FMontagePhaseFlowBinding& Existing = Pair.Value;
		if (Existing.Phase == Phase && Existing.Flow == Flow && Existing.Target == Target)
		{
			return Pair.Key;
		}
	}

	FMontagePhaseFlowBinding Binding;
	Binding.Phase = Phase;
	Binding.Flow = Flow;
	Binding.Target = Target;

	const FGuid Handle = FGuid::NewGuid();
	Bindings.Add(Handle, Binding);

	UE_LOG(LogTemp, Log, TEXT("[PhaseFlow] Bound %s to phase %s on %s."),
		*GetNameSafe(Flow), *Phase.ToString(), *GetNameSafe(GetOwner()));

	return Handle;
}

bool UMontagePhaseFlowComponent::UnbindPhaseFlow(FGuid Handle)
{
	return Bindings.Remove(Handle) > 0;
}

void UMontagePhaseFlowComponent::ClearPhaseFlows()
{
	Bindings.Reset();
}

void UMontagePhaseFlowComponent::HandleGameplayTagChanged(FGameplayTag Tag, bool bAdded)
{
	if (!bAdded || Bindings.IsEmpty())
	{
		return;
	}

	if (!Tag.MatchesTag(MontagePhaseFlow_PhaseRootTag()))
	{
		return;
	}

	LaunchPhase(Tag);
}

void UMontagePhaseFlowComponent::LaunchPhase(const FGameplayTag& Phase)
{
	AActor* Owner = GetOwner();
	if (!IsValid(Owner))
	{
		return;
	}

	for (const TPair<FGuid, FMontagePhaseFlowBinding>& Pair : Bindings)
	{
		const FMontagePhaseFlowBinding& Binding = Pair.Value;
		if (Binding.Phase != Phase || !Binding.Flow)
		{
			continue;
		}

		AActor* TargetActor = ResolveTargetActor(Binding.Target);
		if (!IsValid(TargetActor))
		{
			// Expected during PreAtk for victim-targeted bindings: nothing has been hit yet.
			UE_LOG(LogTemp, Warning, TEXT("[PhaseFlow] Phase=%s skipped %s on %s: target selector resolved to nothing."),
				*Phase.ToString(), *GetNameSafe(Binding.Flow), *GetNameSafe(Owner));
			continue;
		}

		UBuffFlowComponent* TargetBuffFlow = TargetActor->FindComponentByClass<UBuffFlowComponent>();
		if (!TargetBuffFlow)
		{
			UE_LOG(LogTemp, Warning, TEXT("[PhaseFlow] Phase=%s skipped %s: %s has no UBuffFlowComponent."),
				*Phase.ToString(), *GetNameSafe(Binding.Flow), *GetNameSafe(TargetActor));
			continue;
		}

		// bRestartExistingFlow must be true. BuffFlow nodes stay active by design, so the instance
		// from the previous swing is still alive and a plain StartBuffFlow would discard this launch
		// as a duplicate. Restarting aborts that instance first, which is why phase flows have to
		// apply Instant effects: anything an aborted node can undo in Cleanup() will be undone.
		TargetBuffFlow->StartBuffFlow(Binding.Flow, FGuid::NewGuid(), Owner, true);

		UE_LOG(LogTemp, Log, TEXT("[PhaseFlow] Phase=%s launched %s on %s."),
			*Phase.ToString(), *GetNameSafe(Binding.Flow), *GetNameSafe(TargetActor));
	}
}

AActor* UMontagePhaseFlowComponent::ResolveTargetActor(EBFTargetSelector Target) const
{
	AActor* Owner = GetOwner();

	switch (Target)
	{
	case EBFTargetSelector::BuffOwner:
	case EBFTargetSelector::LifecycleTarget:
		return Owner;

	case EBFTargetSelector::LastDamageTarget:
	case EBFTargetSelector::DamageCauser:
	case EBFTargetSelector::AllHitTargets:
	case EBFTargetSelector::BuffGiver:
	{
		const UBuffFlowComponent* OwnerBuffFlow = Owner ? Owner->FindComponentByClass<UBuffFlowComponent>() : nullptr;
		if (!OwnerBuffFlow)
		{
			return nullptr;
		}

		const FBFEventContext& EventContext = OwnerBuffFlow->LastEventContext;
		if (Target == EBFTargetSelector::DamageCauser)
		{
			return EventContext.DamageCauser.Get();
		}

		return EventContext.DamageReceiver.Get();
	}
	}

	return Owner;
}

UYogAbilitySystemComponent* UMontagePhaseFlowComponent::GetOwnerASC() const
{
	AActor* Owner = GetOwner();
	return Owner ? Owner->FindComponentByClass<UYogAbilitySystemComponent>() : nullptr;
}
