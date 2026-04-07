#include "Data/EffectDataAsset.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"

UGameplayEffect* UEffectDataAsset::CreateGameplayEffect(UObject* Outer) const
{
	UGameplayEffect* GE = NewObject<UGameplayEffect>(
		Outer ? Outer : GetTransientPackage(),
		FName(*FString::Printf(TEXT("EDA_GE_%s"), *GetName()))
	);

	// ─── Duration ─────────────────────────────────────────────

	switch (DurationType)
	{
	case ERuneDurationType::Instant:
		GE->DurationPolicy = EGameplayEffectDurationType::Instant;
		break;
	case ERuneDurationType::Infinite:
		GE->DurationPolicy = EGameplayEffectDurationType::Infinite;
		break;
	case ERuneDurationType::Duration:
		GE->DurationPolicy = EGameplayEffectDurationType::HasDuration;
		GE->DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(Duration));
		break;
	}

	if (Period > 0.f)
	{
		GE->Period = FScalableFloat(Period);
	}

	// ─── Stacking ─────────────────────────────────────────────

	switch (UniqueType)
	{
	case ERuneUniqueType::NonUnique:
		GE->StackingType = EGameplayEffectStackingType::None;
		break;
	case ERuneUniqueType::Unique:
		GE->StackingType = EGameplayEffectStackingType::AggregateByTarget;
		break;
	case ERuneUniqueType::BySource:
		GE->StackingType = EGameplayEffectStackingType::AggregateBySource;
		break;
	}

	if (UniqueType != ERuneUniqueType::NonUnique)
	{
		switch (StackType)
		{
		case ERuneStackType::Refresh:
			GE->StackLimitCount            = 1;
			GE->StackDurationRefreshPolicy = EGameplayEffectStackingDurationPolicy::RefreshOnSuccessfulApplication;
			GE->StackPeriodResetPolicy     = EGameplayEffectStackingPeriodPolicy::NeverReset;
			break;
		case ERuneStackType::Stack:
			GE->StackLimitCount            = FMath::Max(1, MaxStack);
			GE->StackDurationRefreshPolicy = EGameplayEffectStackingDurationPolicy::RefreshOnSuccessfulApplication;
			GE->StackPeriodResetPolicy     = EGameplayEffectStackingPeriodPolicy::NeverReset;
			break;
		case ERuneStackType::None:
			GE->StackLimitCount            = 1;
			GE->StackDurationRefreshPolicy = EGameplayEffectStackingDurationPolicy::NeverRefresh;
			GE->StackPeriodResetPolicy     = EGameplayEffectStackingPeriodPolicy::NeverReset;
			break;
		}

		if (StackType != ERuneStackType::None)
		{
			switch (StackReduceType)
			{
			case ERuneStackReduceType::All:
				GE->StackExpirationPolicy = EGameplayEffectStackingExpirationPolicy::ClearEntireStack;
				break;
			case ERuneStackReduceType::One:
				GE->StackExpirationPolicy = EGameplayEffectStackingExpirationPolicy::RemoveSingleStackAndRefreshDuration;
				break;
			}
		}
	}

	// ─── Identity Tag ─────────────────────────────────────────

	if (EffectTag.IsValid())
	{
		GE->InheritableGameplayEffectTags.Added.AddTag(EffectTag);
	}

	// ─── Effects（Modifier / Tag / Cue 等）────────────────────

	for (const URuneEffectFragment* Fragment : Effects)
	{
		if (Fragment)
		{
			Fragment->ApplyToGE(GE);
		}
	}

	return GE;
}

FActiveGameplayEffectHandle UEffectDataAsset::ApplyToSelf(UAbilitySystemComponent* ASC, float Level) const
{
	if (!ASC)
	{
		return FActiveGameplayEffectHandle();
	}

	UGameplayEffect* GE = CreateGameplayEffect(GetTransientPackage());
	if (!GE)
	{
		return FActiveGameplayEffectHandle();
	}

	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	FGameplayEffectSpec Spec(GE, Context, Level);
	return ASC->ApplyGameplayEffectSpecToSelf(Spec);
}

FActiveGameplayEffectHandle UEffectDataAsset::ApplyToTarget(
	UAbilitySystemComponent* SourceASC,
	UAbilitySystemComponent* TargetASC,
	float Level) const
{
	if (!SourceASC || !TargetASC)
	{
		return FActiveGameplayEffectHandle();
	}

	UGameplayEffect* GE = CreateGameplayEffect(GetTransientPackage());
	if (!GE)
	{
		return FActiveGameplayEffectHandle();
	}

	// 以 Source 为来源构建 Context（保留伤害来源信息）
	FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();
	FGameplayEffectSpec Spec(GE, Context, Level);
	return TargetASC->ApplyGameplayEffectSpecToSelf(Spec);
}
