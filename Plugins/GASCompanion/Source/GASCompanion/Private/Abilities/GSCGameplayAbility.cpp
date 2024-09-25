// Copyright 2021 Mickael Daniel. All Rights Reserved.


#include "Abilities/GSCGameplayAbility.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Abilities/GSCTargetType.h"
#include "Components/GSCAbilityQueueComponent.h"
#include "Abilities/GSCBlueprintFunctionLibrary.h"
#include "GSCLog.h"

UGSCGameplayAbility::UGSCGameplayAbility() {}

FGSCGameplayEffectContainerSpec UGSCGameplayAbility::MakeEffectContainerSpecFromContainer(const FGSCGameplayEffectContainer& Container, const FGameplayEventData& EventData, int32 OverrideGameplayLevel)
{
	// First figure out our actor info
	FGSCGameplayEffectContainerSpec ReturnSpec;
	const AActor* OwningActor = GetOwningActorFromActorInfo();
	UAbilitySystemComponent* OwningASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(OwningActor);

	if (OwningASC)
	{
		// If we have a target type, run the targeting logic. This is optional, targets can be added later
		if (Container.TargetType.Get())
		{
			TArray<FHitResult> HitResults;
			TArray<AActor*> TargetActors;
			const UGSCTargetType* TargetTypeCDO = Container.TargetType.GetDefaultObject();
			AActor* AvatarActor = GetAvatarActorFromActorInfo();
			TargetTypeCDO->GetTargets(AvatarActor, EventData, HitResults, TargetActors);
			ReturnSpec.AddTargets(HitResults, TargetActors);
		}

		// If we don't have an override level, use the default on the ability itself
		if (OverrideGameplayLevel == INDEX_NONE)
		{
			OverrideGameplayLevel = GetAbilityLevel();
		}

		// Build GameplayEffectSpecs for each applied effect
		for (const TSubclassOf<UGameplayEffect>& EffectClass : Container.TargetGameplayEffectClasses)
		{
			FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(EffectClass, OverrideGameplayLevel);

			FGameplayEffectSpec* Spec = SpecHandle.Data.Get();
			if (Spec && Container.bUseSetByCallerMagnitude)
			{
				Spec->SetSetByCallerMagnitude(Container.SetByCallerDataTag, Container.SetByCallerMagnitude);
			}
			ReturnSpec.TargetGameplayEffectSpecs.Add(SpecHandle);
		}
	}
	return ReturnSpec;
}

FGSCGameplayEffectContainerSpec UGSCGameplayAbility::MakeEffectContainerSpec(FGameplayTag ContainerTag, const FGameplayEventData& EventData, int32 OverrideGameplayLevel)
{
	FGSCGameplayEffectContainer* FoundContainer = EffectContainerMap.Find(ContainerTag);

	if (FoundContainer)
	{
		return MakeEffectContainerSpecFromContainer(*FoundContainer, EventData, OverrideGameplayLevel);
	}
	return FGSCGameplayEffectContainerSpec();
}

TArray<FActiveGameplayEffectHandle> UGSCGameplayAbility::ApplyEffectContainerSpec(const FGSCGameplayEffectContainerSpec& ContainerSpec)
{
	TArray<FActiveGameplayEffectHandle> AllEffects;

	// Iterate list of effect specs and apply them to their target data
	for (const FGameplayEffectSpecHandle& SpecHandle : ContainerSpec.TargetGameplayEffectSpecs)
	{
		AllEffects.Append(K2_ApplyGameplayEffectSpecToTarget(SpecHandle, ContainerSpec.TargetData));
	}
	return AllEffects;
}

TArray<FActiveGameplayEffectHandle> UGSCGameplayAbility::ApplyEffectContainer(FGameplayTag ContainerTag, const FGameplayEventData& EventData, int32 OverrideGameplayLevel)
{
	const FGSCGameplayEffectContainerSpec Spec = MakeEffectContainerSpec(ContainerTag, EventData, OverrideGameplayLevel);
	return ApplyEffectContainerSpec(Spec);
}

void UGSCGameplayAbility::OnAvatarSet(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	Super::OnAvatarSet(ActorInfo, Spec);

	if (bActivateOnGranted)
	{
		ActorInfo->AbilitySystemComponent->TryActivateAbility(Spec.Handle, false);
	}
}

bool UGSCGameplayAbility::CheckCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (bLooselyCheckAbilityCost)
	{
		return true;
	}

	return Super::CheckCost(Handle, ActorInfo, OptionalRelevantTags);
}

bool UGSCGameplayAbility::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	const bool bCanActivateAbility = Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags);
	if (!bCanActivateAbility)
	{
		return false;
	}

	if (!bHasBlueprintCanUse && bLooselyCheckAbilityCost)
	{
		return CheckForPositiveCost(Handle, ActorInfo, OptionalRelevantTags);
	}

	return true;
}

void UGSCGameplayAbility::PreActivate(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, FOnGameplayAbilityEnded::FDelegate* OnGameplayAbilityEndedDelegate, const FGameplayEventData* TriggerEventData)
{
	Super::PreActivate(Handle, ActorInfo, ActivationInfo, OnGameplayAbilityEndedDelegate);
	OnGameplayAbilityEnded.AddUObject(this, &UGSCGameplayAbility::AbilityEnded);

	// Open ability queue only if told to do so
	if (!bEnableAbilityQueue)
	{
		return;
	}

	AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!Avatar)
	{
		return;
	}

	UGSCAbilityQueueComponent* AbilityQueueComponent = UGSCBlueprintFunctionLibrary::GetAbilityQueueComponent(Avatar);
	if (!AbilityQueueComponent)
	{
		return;
	}

	GSC_LOG(Log, TEXT("UGSCGameplayAbility::PreActivate %s, Open Ability Queue"), *GetName())
	AbilityQueueComponent->OpenAbilityQueue();
	AbilityQueueComponent->SetAllowAllAbilitiesForAbilityQueue(true);
}

void UGSCGameplayAbility::AbilityEnded(UGameplayAbility* Ability)
{
	GSC_LOG(Log, TEXT("UGSCGameplayAbility::AbilityEnded"))
	AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!Avatar)
	{
		GSC_LOG(Warning, TEXT("UGSCGameplayAbility::AbilityEnded Coundl't get avatar actor from actor info"))
		return;
	}

	GSC_LOG(Log, TEXT("UGSCGameplayAbility::AbilityEnded Broadcast"))
	// Trigger OnEndDelegate now in case Blueprints need it
	OnAbilityEnded.Broadcast();
	OnAbilityEnded.Clear();
}

bool UGSCGameplayAbility::CheckForPositiveCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags) const
{
	UGameplayEffect* CostGE = GetCostGameplayEffect();
	if (!CostGE)
	{
		return true;
	}

	if (!CanApplyPositiveAttributeModifiers(CostGE, ActorInfo, GetAbilityLevel(Handle, ActorInfo), MakeEffectContext(Handle, ActorInfo)))
	{
		const FGameplayTag& CostTag = UAbilitySystemGlobals::Get().ActivateFailCostTag;
		if (OptionalRelevantTags && CostTag.IsValid())
		{
			OptionalRelevantTags->AddTag(CostTag);
		}

		return false;
	}

	return true;
}

bool UGSCGameplayAbility::CanApplyPositiveAttributeModifiers(const UGameplayEffect* GameplayEffect, const FGameplayAbilityActorInfo* ActorInfo, const float Level, const FGameplayEffectContextHandle& EffectContext) const
{
	FGameplayEffectSpec	Spec(GameplayEffect, EffectContext, Level);

	Spec.CalculateModifierMagnitudes();

	UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.IsValid() ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	if (!ASC)
	{
		return false;
	}

	for(int32 ModIdx = 0; ModIdx < Spec.Modifiers.Num(); ++ModIdx)
	{
		const FGameplayModifierInfo& ModDef = Spec.Def->Modifiers[ModIdx];

		// It only makes sense to check additive operators
		if (ModDef.ModifierOp == EGameplayModOp::Additive)
		{
			if (!ModDef.Attribute.IsValid())
			{
				continue;
			}

			if (!ASC->HasAttributeSetForAttribute(ModDef.Attribute))
			{
				continue;
			}

			const UAttributeSet* Set = GetAttributeSubobjectForASC(ASC, ModDef.Attribute.GetAttributeSetClass());
			const float CurrentValue = ModDef.Attribute.GetNumericValueChecked(Set);

			if (CurrentValue <= 0.f)
			{
				return false;
			}
		}
	}

	return true;
}

const UAttributeSet* UGSCGameplayAbility::GetAttributeSubobjectForASC(UAbilitySystemComponent* AbilitySystemComponent, const TSubclassOf<UAttributeSet> AttributeClass)
{
	check(AbilitySystemComponent != nullptr);

	for (const UAttributeSet* Set : AbilitySystemComponent->GetSpawnedAttributes())
	{
		if (Set && Set->IsA(AttributeClass))
		{
			return Set;
		}
	}

	return nullptr;
}
