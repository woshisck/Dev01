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

	// --- Duration Type ---
	switch (RC.DurationType)
	{
	case ERuneDurationType::Instant:
		GE->DurationPolicy = EGameplayEffectDurationType::Instant;
		break;
	case ERuneDurationType::Infinite:
		GE->DurationPolicy = EGameplayEffectDurationType::Infinite;
		break;
	case ERuneDurationType::Duration:
		GE->DurationPolicy = EGameplayEffectDurationType::HasDuration;
		GE->DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(RC.Duration));
		break;
	}

	// --- Period（>0 时周期触发，适用于 DoT / HoT）---
	if (RC.Period > 0.f)
	{
		GE->Period = FScalableFloat(RC.Period);
	}

	// --- UniqueType → GAS StackingType 基础 ---
	switch (RC.UniqueType)
	{
	case ERuneUniqueType::NonUnique:
		// 非唯一：每次施加都是独立 GE 实例，StackType 不生效
		GE->StackingType = EGameplayEffectStackingType::None;
		break;

	case ERuneUniqueType::Unique:
		GE->StackingType = EGameplayEffectStackingType::AggregateByTarget;
		break;

	case ERuneUniqueType::BySource:
		GE->StackingType = EGameplayEffectStackingType::AggregateBySource;
		break;
	}

	// --- StackType（唯一/源唯一 时进一步配置堆叠行为）---
	if (RC.UniqueType != ERuneUniqueType::NonUnique)
	{
		switch (RC.StackType)
		{
		case ERuneStackType::Refresh:
			// 刷新：不叠加层数，到期时间刷新
			GE->StackLimitCount            = 1;
			GE->StackDurationRefreshPolicy = EGameplayEffectStackingDurationPolicy::RefreshOnSuccessfulApplication;
			GE->StackPeriodResetPolicy     = EGameplayEffectStackingPeriodPolicy::NeverReset;
			break;

		case ERuneStackType::Stack:
			// 叠加：增加层数（至 MaxStack），刷新持续时间
			GE->StackLimitCount            = FMath::Max(1, RC.MaxStack);
			GE->StackDurationRefreshPolicy = EGameplayEffectStackingDurationPolicy::RefreshOnSuccessfulApplication;
			GE->StackPeriodResetPolicy     = EGameplayEffectStackingPeriodPolicy::NeverReset;
			break;

		case ERuneStackType::None:
			// 禁止：不叠加，不刷新时间
			GE->StackLimitCount            = 1;
			GE->StackDurationRefreshPolicy = EGameplayEffectStackingDurationPolicy::NeverRefresh;
			GE->StackPeriodResetPolicy     = EGameplayEffectStackingPeriodPolicy::NeverReset;
			break;
		}

		// --- StackReduceType（到期减层方式，禁止堆叠时无意义）---
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
	}

	// --- RuneTag（GE 资产标签，供 GetRuneInfo / RemoveRune 按 Tag 查询）---
	if (RC.RuneTag.IsValid())
		GE->InheritableGameplayEffectTags.Added.AddTag(RC.RuneTag);

	// --- Effects Fragments（属性修改 / Tag / Cue 等）---
	for (const URuneEffectFragment* Fragment : RC.Effects)
	{
		if (Fragment)
			Fragment->ApplyToGE(GE);
	}

	return GE;
}
