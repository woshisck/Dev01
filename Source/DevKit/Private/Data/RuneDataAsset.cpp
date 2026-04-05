#include "Data/RuneDataAsset.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/Abilities/YogGameplayAbility.h"

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

// ------------------------------------------------------------

static float GetAttrValue(UAbilitySystemComponent* ASC, const FGameplayAttribute& Attr)
{
	if (!ASC || !Attr.IsValid()) return 0.f;
	bool bFound = false;
	const float Val = ASC->GetGameplayAttributeValue(Attr, bFound);
	return bFound ? Val : 0.f;
}

UGameplayEffect* FRuneInstance::CreateTransientGE(UObject* Outer, UAbilitySystemComponent* ASC) const
{
	UGameplayEffect* GE = NewObject<UGameplayEffect>(
		Outer ? Outer : GetTransientPackage(),
		FName(*FString::Printf(TEXT("RuneGE_%s"), *RuneName.ToString()))
	);

	const FRuneBuffConfig& BC = BuffConfig;
	const FRuneValues& V = Values;

	// --- Duration ---
	GE->DurationPolicy = BC.DurationPolicy;
	if (BC.DurationPolicy == EGameplayEffectDurationType::HasDuration)
		GE->DurationMagnitude = BC.DurationMagnitude;
	if (BC.DurationPolicy != EGameplayEffectDurationType::Instant)
	{
		GE->Period = BC.Period;
		GE->bExecutePeriodicEffectOnApplication = BC.bExecutePeriodicEffectOnApplication;
		GE->PeriodicInhibitionPolicy = BC.PeriodicInhibitionPolicy;
	}

	// --- Modifiers ---

	// 1. 简化修改器
	for (const FRuneAttributeModifier& AM : V.AttributeModifiers)
	{
		if (!AM.Attribute.IsValid()) continue;
		FGameplayModifierInfo ModInfo;
		ModInfo.Attribute  = AM.Attribute;
		ModInfo.ModifierOp = AM.ModOp;
		ModInfo.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(AM.Value));
		GE->Modifiers.Add(ModInfo);
	}

	// 2. 高级修改器
	GE->Modifiers.Append(V.Modifiers);

	// 3. CalcSpec 公式（快照当前 ASC 属性值）
	for (const FRuneCalcSpec& Spec : V.CalcSpecs)
	{
		if (!Spec.OutputAttribute.IsValid() || !Spec.AttributeA.IsValid()) continue;

		const float A = GetAttrValue(ASC, Spec.AttributeA);
		const float B = (Spec.Operation != ERuneCalcOp::UseA) ? GetAttrValue(ASC, Spec.AttributeB) : 0.f;

		float Raw = 0.f;
		switch (Spec.Operation)
		{
		case ERuneCalcOp::UseA:      Raw = A;      break;
		case ERuneCalcOp::A_Minus_B: Raw = A - B;  break;
		case ERuneCalcOp::A_Plus_B:  Raw = A + B;  break;
		case ERuneCalcOp::A_Times_B: Raw = A * B;  break;
		}

		FGameplayModifierInfo CalcMod;
		CalcMod.Attribute  = Spec.OutputAttribute;
		CalcMod.ModifierOp = Spec.ModOp;
		CalcMod.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(Raw * Spec.Coefficient + Spec.Addend));
		GE->Modifiers.Add(CalcMod);
	}

	// --- Executions ---
	GE->Executions = V.Executions;

	// --- Stacking ---
	GE->StackingType                = BC.StackingType;
	GE->StackLimitCount             = BC.StackLimitCount;
	GE->StackDurationRefreshPolicy  = BC.StackDurationRefreshPolicy;
	GE->StackPeriodResetPolicy      = BC.StackPeriodResetPolicy;
	GE->StackExpirationPolicy       = BC.StackExpirationPolicy;
	GE->OverflowEffects             = BC.OverflowEffects;
	GE->bDenyOverflowApplication    = BC.bDenyOverflowApplication;
	GE->bClearStackOnOverflow       = BC.bClearStackOnOverflow;

	// --- Cues ---
	GE->bRequireModifierSuccessToTriggerCues = V.GameplayCues.Num() > 0 ? false : false;
	GE->GameplayCues = V.GameplayCues;

	// --- Tags ---
	if (BC.GrantedTagsToTarget.Num() > 0)
		GE->InheritableOwnedTagsContainer.Added.AppendTags(BC.GrantedTagsToTarget);
	if (BC.BuffTag.IsValid())
		GE->InheritableGameplayEffectTags.Added.AddTag(BC.BuffTag);

	return GE;
}
