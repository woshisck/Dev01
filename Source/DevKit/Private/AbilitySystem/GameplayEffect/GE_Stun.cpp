#include "AbilitySystem/GameplayEffect/GE_Stun.h"

#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"
#include "GameplayTagContainer.h"

UGE_Stun::UGE_Stun()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(1.5f));

	if (const FGameplayTag StunnedTag = FGameplayTag::RequestGameplayTag(TEXT("Buff.Status.Stunned"), false);
		StunnedTag.IsValid())
	{
		FInheritedTagContainer GrantedTagChanges;
		GrantedTagChanges.Added.AddTag(StunnedTag);
		UTargetTagsGameplayEffectComponent* TargetTagsComponent =
			CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(TEXT("TargetTags"));
		if (TargetTagsComponent)
		{
			GEComponents.Add(TargetTagsComponent);
			TargetTagsComponent->SetAndApplyTargetTagChanges(GrantedTagChanges);
		}
	}

	StackingType = EGameplayEffectStackingType::AggregateByTarget;
	StackLimitCount = 1;
	StackDurationRefreshPolicy = EGameplayEffectStackingDurationPolicy::RefreshOnSuccessfulApplication;
	StackExpirationPolicy = EGameplayEffectStackingExpirationPolicy::ClearEntireStack;
}
