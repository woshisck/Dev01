#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "ActionRules/DevKitActionRuleSubsystem.h"
#include "Character/PlayerCharacterBase.h"
#include "Character/EnemyCharacterBase.h"
#include "Engine/World.h"

FDevKitActionRuleDecision UYogAbilitySystemComponent::QueryActionRules(const FGameplayTagContainer& RequestTags) const
{
	const UWorld* World = GetWorld();
	const auto* Rules = World ? World->GetSubsystem<UDevKitActionRuleSubsystem>() : nullptr;
	if (!Rules) return {};
	FString Subject = TEXT("other");
	if (Cast<APlayerCharacterBase>(GetAvatarActor())) Subject = TEXT("player");
	else if (Cast<AEnemyCharacterBase>(GetAvatarActor())) Subject = TEXT("enemy");
	auto Decision = Rules->Evaluate(RequestTags, GetOwnedGameplayTags(), Subject, ActionRuleValues);
	for (const FGameplayAbilitySpec& Spec : GetActivatableAbilities())
	{
		if (!Decision.CancelAbilityTags.IsEmpty() && Spec.Ability && Spec.Ability->GetAssetTags() == RequestTags
			&& Spec.Ability->GetInstancingPolicy() == EGameplayAbilityInstancingPolicy::NonInstanced)
		{
			Decision.bBlocked = Decision.bError = true;
			// The GAS tag-admission hook has no requester handle. Reject ambiguous mixed
			// policies instead of allowing a NonInstanced request to miss its cancel plan.
			Decision.Reasons.Add(TEXT("All granted abilities sharing this action identity must be instanced; conflicting ability: ") + Spec.Ability->GetName());
		}
		if (!Spec.IsActive() || !Spec.Ability || !Spec.Ability->GetAssetTags().HasAny(Decision.CancelAbilityTags)) continue;
		if (Spec.Ability->GetInstancingPolicy() == EGameplayAbilityInstancingPolicy::NonInstanced && !Spec.Ability->CanBeCanceled())
		{
			Decision.bBlocked = true;
			Decision.Reasons.Add(TEXT("Interrupt target is not cancelable: ") + Spec.Ability->GetName());
		}
		for (const UGameplayAbility* Instance : Spec.GetAbilityInstances())
			if (Instance && Instance->IsActive() && !Instance->CanBeCanceled())
			{
				Decision.bBlocked = true;
				Decision.Reasons.Add(TEXT("Interrupt target is not cancelable: ") + Instance->GetName());
			}
	}
	if (Decision.bBlocked) Decision.CancelAbilityTags.Reset();
	return Decision;
}

bool UYogAbilitySystemComponent::AreAbilityTagsBlocked(const FGameplayTagContainer& Tags) const
{
	// Never grant immunity to native GAS, cost, cooldown, death or existing locks.
	return Super::AreAbilityTagsBlocked(Tags) || QueryActionRules(Tags).bBlocked;
}

void UYogAbilitySystemComponent::ApplyAbilityBlockAndCancelTags(const FGameplayTagContainer& AbilityTags,
	UGameplayAbility* RequestingAbility, bool bEnableBlockTags, const FGameplayTagContainer& BlockTags,
	bool bExecuteCancelTags, const FGameplayTagContainer& CancelTags)
{
	FGameplayTagContainer EffectiveCancel = CancelTags;
	if (bExecuteCancelTags)
	{
		// PreActivate has already added incoming owned tags here. Consume the earlier snapshot;
		// reevaluation here would confuse the newly entered state with the pre-request state.
		FDevKitActionRuleDecision Decision;
		if (PendingActionRuleActivations.RemoveAndCopyValue(RequestingAbility, Decision) && !Decision.bBlocked)
			EffectiveCancel.AppendTags(Decision.CancelAbilityTags);
	}
	Super::ApplyAbilityBlockAndCancelTags(AbilityTags, RequestingAbility, bEnableBlockTags,
		BlockTags, bExecuteCancelTags, EffectiveCancel);
}

void UYogAbilitySystemComponent::HandleChangeAbilityCanBeCanceled(const FGameplayTagContainer& AbilityTags,
	UGameplayAbility* RequestingAbility, bool bCanBeCanceled)
{
	Super::HandleChangeAbilityCanBeCanceled(AbilityTags, RequestingAbility, bCanBeCanceled);
	// UE5.8 PreActivate calls this before ActivationOwnedTags and NotifyAbilityActivated.
	// SetCanBeCanceled on an already active instance must not replace the pending snapshot.
	if (bCanBeCanceled && RequestingAbility && RequestingAbility->IsInstantiated() && !ActiveActionRuleInstances.Contains(RequestingAbility))
	{
		PendingActionRuleActivations.Add(RequestingAbility, QueryActionRules(AbilityTags));
	}
}

void UYogAbilitySystemComponent::NotifyAbilityActivated(const FGameplayAbilitySpecHandle Handle, UGameplayAbility* Ability)
{
	if (Ability && Ability->IsInstantiated()) ActiveActionRuleInstances.Add(Ability);
	Super::NotifyAbilityActivated(Handle, Ability);
}

void UYogAbilitySystemComponent::NotifyAbilityEnded(FGameplayAbilitySpecHandle Handle, UGameplayAbility* Ability, bool bWasCancelled)
{
	PendingActionRuleActivations.Remove(Ability);
	ActiveActionRuleInstances.Remove(Ability);
	Super::NotifyAbilityEnded(Handle, Ability, bWasCancelled);
}

bool UYogAbilitySystemComponent::SetActionRuleValue(const FString& Name, double Value)
{
	if (Name.IsEmpty() || Name.Len() > 64 || !FMath::IsFinite(Value) || (!ActionRuleValues.Contains(Name) && ActionRuleValues.Num() >= 128)) return false;
	for (int32 Index = 0; Index < Name.Len(); ++Index)
	{
		const TCHAR C = Name[Index];
		if (!((C >= TEXT('a') && C <= TEXT('z')) || (C >= TEXT('A') && C <= TEXT('Z')) || C == TEXT('_')
			|| (Index > 0 && C >= TEXT('0') && C <= TEXT('9')))) return false;
	}
	ActionRuleValues.Add(Name, Value); return true;
}

void UYogAbilitySystemComponent::ClearActionRuleValue(const FString& Name)
{
	ActionRuleValues.Remove(Name);
}
