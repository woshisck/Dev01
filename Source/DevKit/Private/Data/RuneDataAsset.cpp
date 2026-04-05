#include "Data/RuneDataAsset.h"

FRuneShape FRuneShape::Rotate90() const
{
	FRuneShape Result;
	for (const FIntPoint& Cell : Cells)
	{
		Result.Cells.Add(FIntPoint(-Cell.Y, Cell.X));
	}
	if (Result.Cells.IsEmpty())
		return Result;

	int32 MinX = INT32_MAX, MinY = INT32_MAX;
	for (const FIntPoint& Cell : Result.Cells)
	{
		MinX = FMath::Min(MinX, Cell.X);
		MinY = FMath::Min(MinY, Cell.Y);
	}
	for (FIntPoint& Cell : Result.Cells)
	{
		Cell.X -= MinX;
		Cell.Y -= MinY;
	}
	return Result;
}

FRuneInstance URuneDataAsset::CreateInstance() const
{
	FRuneInstance Instance = RuneTemplate;
	Instance.RuneGuid = FGuid::NewGuid();
	Instance.SourceDA = const_cast<URuneDataAsset*>(this);
	return Instance;
}

UGameplayEffect* URuneDataAsset::CreateTransientGE(UObject* Outer) const
{
	const FRuneConfig& RC = RuneTemplate.RuneConfig;

	UGameplayEffect* GE = NewObject<UGameplayEffect>(
		Outer ? Outer : GetTransientPackage(),
		FName(*FString::Printf(TEXT("RuneGE_%s"), *RuneTemplate.RuneName.ToString()))
	);

	// --- Duration（BuffDuration: 0=瞬发, <0=永久, >0=有时限）---
	if (RC.BuffDuration == 0.f)
	{
		GE->DurationPolicy = EGameplayEffectDurationType::Instant;
	}
	else if (RC.BuffDuration < 0.f)
	{
		GE->DurationPolicy = EGameplayEffectDurationType::Infinite;
	}
	else
	{
		GE->DurationPolicy = EGameplayEffectDurationType::HasDuration;
		GE->DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(RC.BuffDuration));
	}

	// --- Stacking ---
	const int32 ActualMaxStack = FMath::Max(1, RC.MaxStack);

	switch (RC.StackType)
	{
	case ERuneStackType::None:
		GE->StackingType = EGameplayEffectStackingType::None;
		break;

	case ERuneStackType::Refresh:
		// 刷新：不叠加层数（StackLimitCount=1），到期时间刷新
		GE->StackingType               = EGameplayEffectStackingType::AggregateByTarget;
		GE->StackLimitCount            = 1;
		GE->StackDurationRefreshPolicy = EGameplayEffectStackingDurationPolicy::RefreshOnSuccessfulApplication;
		GE->StackPeriodResetPolicy     = EGameplayEffectStackingPeriodPolicy::NeverReset;
		break;

	case ERuneStackType::Stack:
		// 叠加：增加层数（至 MaxStack），刷新持续时间
		GE->StackingType               = EGameplayEffectStackingType::AggregateByTarget;
		GE->StackLimitCount            = ActualMaxStack;
		GE->StackDurationRefreshPolicy = EGameplayEffectStackingDurationPolicy::RefreshOnSuccessfulApplication;
		GE->StackPeriodResetPolicy     = EGameplayEffectStackingPeriodPolicy::NeverReset;
		break;
	}

	// --- StackReduceType（到期减层方式）---
	if (RC.StackType != ERuneStackType::None)
	{
		switch (RC.StackReduceType)
		{
		case ERuneStackReduceType::All:
			GE->StackExpirationPolicy = EGameplayEffectStackingExpirationPolicy::ClearEntireStack;
			break;
		case ERuneStackReduceType::One:
			GE->StackExpirationPolicy = EGameplayEffectStackingExpirationPolicy::RemoveSingleStackAndRefreshDuration;
			break;
		}
	}

	// --- BuffTag（Asset Tag，供 GetRuneInfo 节点按 Tag 查询）---
	if (RC.BuffTag.IsValid())
		GE->InheritableGameplayEffectTags.Added.AddTag(RC.BuffTag);

	// --- Effects Fragments（属性修改 / Tag / Cue 等）---
	for (const URuneEffectFragment* Fragment : RC.Effects)
	{
		if (Fragment)
			Fragment->ApplyToGE(GE);
	}

	return GE;
}
