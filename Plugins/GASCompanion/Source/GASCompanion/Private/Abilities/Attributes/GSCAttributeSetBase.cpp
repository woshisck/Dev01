// Copyright 2021 Mickael Daniel. All Rights Reserved.

#include "Abilities/Attributes/GSCAttributeSetBase.h"

#include "GSCLog.h"
#include "GameplayEffectExtension.h"
#include "Abilities/GSCBlueprintFunctionLibrary.h"
#include "Components/GSCCoreComponent.h"
#include "GameFramework/Pawn.h"

// Sets default values
UGSCAttributeSetBase::UGSCAttributeSetBase()
{
	// ...
}

void UGSCAttributeSetBase::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
    // This is called whenever attributes change, so for max attributes we want to scale the current totals to match
    Super::PreAttributeChange(Attribute, NewValue);

    const FGameplayAbilityActorInfo* ActorInfo = GetActorInfo();
	if (!ActorInfo)
	{
		return;
	}

    const TWeakObjectPtr<AActor> AvatarActorPtr = ActorInfo->AvatarActor;
	if (!AvatarActorPtr.IsValid())
	{
		return;
	}

    const AActor* AvatarActor = AvatarActorPtr.Get();
    UGSCCoreComponent* CoreComponent = UGSCBlueprintFunctionLibrary::GetCompanionCoreComponent(AvatarActor);
    if (CoreComponent)
    {
        CoreComponent->PreAttributeChange(this, Attribute, NewValue);
    }
}

void UGSCAttributeSetBase::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
    Super::PostGameplayEffectExecute(Data);

    AActor* SourceActor = nullptr;
    AActor* TargetActor = nullptr;
    GetSourceAndTargetFromContext<AActor>(Data, SourceActor, TargetActor);

    UGSCCoreComponent* TargetCoreComponent = UGSCBlueprintFunctionLibrary::GetCompanionCoreComponent(TargetActor);
    if (TargetCoreComponent)
    {
        TargetCoreComponent->PostGameplayEffectExecute(this, Data);
    }
}

void UGSCAttributeSetBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}

float UGSCAttributeSetBase::GetClampMinimumValueFor(const FGameplayAttribute& Attribute)
{
	// Subclass are expected to override this method for anything other than 0.f (if GetClampMinimumValueFor() is even used)
	return 0.f;
}

const FGameplayTagContainer& UGSCAttributeSetBase::GetSourceTagsFromContext(const FGameplayEffectModCallbackData& Data)
{
    return *Data.EffectSpec.CapturedSourceTags.GetAggregatedTags();
}

void UGSCAttributeSetBase::AdjustAttributeForMaxChange(FGameplayAttributeData& AffectedAttribute, const FGameplayAttributeData& MaxAttribute, const float NewMaxValue, const FGameplayAttribute& AffectedAttributeProperty) const
{
    UAbilitySystemComponent* AbilitySystemComponent = GetOwningAbilitySystemComponent();
	if (!AbilitySystemComponent)
	{
		return;
	}

    const float CurrentMaxValue = MaxAttribute.GetCurrentValue();
	const float CurrentValue = AffectedAttribute.GetCurrentValue();

    if (!FMath::IsNearlyEqual(CurrentMaxValue, NewMaxValue) && CurrentMaxValue > 0.f)
    {
    	// Get the current relative percent
    	const float Ratio = CurrentValue / CurrentMaxValue;

    	// Calculate value for the affected attribute based on current ratio
    	const float NewValue = FMath::RoundToFloat(NewMaxValue * Ratio);

        GSC_LOG(Verbose, TEXT("AdjustAttributeForMaxChange: CurrentValue: %f, CurrentMaxValue: %f, NewMaxValue: %f, NewValue: %f (Ratio: %f)"), CurrentValue, CurrentMaxValue, NewMaxValue, NewValue, Ratio)
        GSC_LOG(Verbose, TEXT("AdjustAttributeForMaxChange: ApplyModToAttribute %s with %f"), *AffectedAttributeProperty.GetName(), NewValue)
        AbilitySystemComponent->ApplyModToAttribute(AffectedAttributeProperty, EGameplayModOp::Override, NewValue);
    }
}

void UGSCAttributeSetBase::GetExecutionDataFromMod(const FGameplayEffectModCallbackData& Data, FGSCAttributeSetExecutionData& OutExecutionData)
{
	OutExecutionData.Context = Data.EffectSpec.GetContext();
	OutExecutionData.SourceASC = OutExecutionData.Context.GetOriginalInstigatorAbilitySystemComponent();
	OutExecutionData.SourceTags = *Data.EffectSpec.CapturedSourceTags.GetAggregatedTags();
	Data.EffectSpec.GetAllAssetTags(OutExecutionData.SpecAssetTags);

	OutExecutionData.TargetActor = Data.Target.AbilityActorInfo->AvatarActor.IsValid() ? Data.Target.AbilityActorInfo->AvatarActor.Get() : nullptr;
	OutExecutionData.TargetController = Data.Target.AbilityActorInfo->PlayerController.IsValid() ? Data.Target.AbilityActorInfo->PlayerController.Get() : nullptr;
	OutExecutionData.TargetPawn = Cast<APawn>(OutExecutionData.TargetActor);
	OutExecutionData.TargetCoreComponent = UGSCBlueprintFunctionLibrary::GetCompanionCoreComponent(OutExecutionData.TargetActor);

	if (OutExecutionData.SourceASC && OutExecutionData.SourceASC->AbilityActorInfo.IsValid())
	{
		// Get the Source actor, which should be the damage causer (instigator)
		if (OutExecutionData.SourceASC->AbilityActorInfo->AvatarActor.IsValid())
		{
			// Set the source actor based on context if it's set
			if (OutExecutionData.Context.GetEffectCauser())
			{
				OutExecutionData.SourceActor = OutExecutionData.Context.GetEffectCauser();
			}
			else
			{
				OutExecutionData.SourceActor = OutExecutionData.SourceASC->AbilityActorInfo->AvatarActor.IsValid()
					? OutExecutionData.SourceASC->AbilityActorInfo->AvatarActor.Get()
					: nullptr;
			}
		}

		OutExecutionData.SourceController = OutExecutionData.SourceASC->AbilityActorInfo->PlayerController.IsValid()
			? OutExecutionData.SourceASC->AbilityActorInfo->PlayerController.Get()
			: nullptr;
		OutExecutionData.SourcePawn = Cast<APawn>(OutExecutionData.SourceActor);
		OutExecutionData.SourceCoreComponent = UGSCBlueprintFunctionLibrary::GetCompanionCoreComponent(OutExecutionData.SourceActor);
	}

	OutExecutionData.SourceObject = Data.EffectSpec.GetEffectContext().GetSourceObject();

	// Compute the delta between old and new, if it is available
	OutExecutionData.DeltaValue = 0.f;
	if (Data.EvaluatedData.ModifierOp == EGameplayModOp::Type::Additive)
	{
		// If this was additive, store the raw delta value to be passed along later
		OutExecutionData.DeltaValue = Data.EvaluatedData.Magnitude;
	}
}
