#include "Data/RuneDataAsset.h"

#include "Math/BasicMathExpressionEvaluator.h"

namespace
{
	float EvaluateRuneTuningFormula(const URuneDataAsset* Rune, const FRuneTuningScalar& Scalar)
	{
		FString Expression = Scalar.FormulaExpression;
		if (Expression.IsEmpty())
		{
			return Scalar.Value;
		}

		const float Level = Rune ? static_cast<float>(Rune->RuneInfo.Level) : 1.f;
		const float UpgradeLevel = Rune ? static_cast<float>(Rune->RuneInfo.UpgradeLevel) : 0.f;
		Expression.ReplaceInline(TEXT("UpgradeLevel"), *FString::SanitizeFloat(UpgradeLevel), ESearchCase::IgnoreCase);
		Expression.ReplaceInline(TEXT("Level"), *FString::SanitizeFloat(Level), ESearchCase::IgnoreCase);
		Expression.ReplaceInline(TEXT("Value"), *FString::SanitizeFloat(Scalar.Value), ESearchCase::IgnoreCase);

		const FBasicMathExpressionEvaluator Evaluator;
		const TValueOrError<double, FExpressionError> Result = Evaluator.Evaluate(*Expression, Scalar.Value);
		return Result.IsValid() ? static_cast<float>(Result.GetValue()) : Scalar.Value;
	}
}

float URuneValueCalculation::CalculateValue_Implementation(const URuneDataAsset* Rune, FName Key, float DefaultValue) const
{
	return DefaultValue;
}

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

FIntPoint FRuneShape::GetPivotOffset(int32 NumRotations) const
{
    NumRotations %= 4;
    if (NumRotations == 0 || Cells.IsEmpty()) return FIntPoint(0, 0);

    FIntPoint Pivot(0, 0);
    FRuneShape Cur = *this;

    for (int32 i = 0; i < NumRotations; i++)
    {
        int32 MinX = INT32_MAX, MinY = INT32_MAX;
        for (const FIntPoint& C : Cur.Cells)
        {
            MinX = FMath::Min(MinX, -C.Y);
            MinY = FMath::Min(MinY, C.X);
        }
        const FIntPoint RawP(-Pivot.Y, Pivot.X);
        Pivot = FIntPoint(RawP.X - MinX, RawP.Y - MinY);
        Cur = Cur.Rotate90();
    }
    return Pivot;
}

FRuneInstance URuneDataAsset::CreateInstance() const
{
	FRuneInstance Instance = RuneInfo;
	Instance.RuneGuid = FGuid::NewGuid();
	Instance.SourceDA = const_cast<URuneDataAsset*>(this);
	return Instance;
}

bool URuneDataAsset::GetRuneTuningScalar(FName Key, FRuneTuningScalar& OutScalar) const
{
	if (Key.IsNone())
	{
		return false;
	}

	for (const FRuneTuningScalar& Scalar : RuneInfo.RuneConfig.TuningScalars)
	{
		if (Scalar.Key == Key)
		{
			OutScalar = Scalar;
			return true;
		}
	}

	return false;
}

float URuneDataAsset::GetRuneTuningValue(FName Key, float DefaultValue) const
{
	FRuneTuningScalar Scalar;
	if (!GetRuneTuningScalar(Key, Scalar))
	{
		return DefaultValue;
	}

	if (Scalar.ValueSource == ERuneTuningValueSource::MMC && Scalar.MagnitudeCalculationClass)
	{
		if (const URuneValueCalculation* Calculation = Scalar.MagnitudeCalculationClass->GetDefaultObject<URuneValueCalculation>())
		{
			return Calculation->CalculateValue(this, Key, Scalar.Value);
		}
	}

	if (Scalar.ValueSource == ERuneTuningValueSource::Formula)
	{
		return EvaluateRuneTuningFormula(this, Scalar);
	}

	// Context rows keep Value as their editor fallback until runtime context is supplied.
	return Scalar.Value;
}

float URuneDataAsset::GetRuneTuningValue(FName Key, const FRuneTuningResolveContext& Context, float DefaultValue) const
{
	FRuneTuningScalar Scalar;
	if (!GetRuneTuningScalar(Key, Scalar))
	{
		return DefaultValue;
	}

	float BaseValue = DefaultValue;
	if (Scalar.ValueSource == ERuneTuningValueSource::MMC && Scalar.MagnitudeCalculationClass)
	{
		if (const URuneValueCalculation* Calc = Scalar.MagnitudeCalculationClass->GetDefaultObject<URuneValueCalculation>())
		{
			BaseValue = Calc->CalculateValue(this, Key, Scalar.Value);
		}
		else
		{
			BaseValue = Scalar.Value;
		}
	}
	else if (Scalar.ValueSource == ERuneTuningValueSource::Formula)
	{
		BaseValue = EvaluateRuneTuningFormula(this, Scalar);
	}
	else
	{
		BaseValue = Scalar.Value;
	}

	const FRuneComboBonusConfig& CB = Scalar.ComboBonus;
	if (CB.IsEnabled())
	{
		const int32 ComboStacks = FMath::Max(0, Context.ComboIndex - 1);
		const float Bonus = (CB.MaxBonus > 0.f)
			? FMath::Min(CB.MaxBonus, ComboStacks * CB.BonusPerStack)
			: ComboStacks * CB.BonusPerStack;

		if (CB.Mode == ERuneComboBonusMode::Add)
		{
			BaseValue += Bonus;
		}
		else if (CB.Mode == ERuneComboBonusMode::Multiply)
		{
			BaseValue *= (1.f + Bonus);
		}

		switch (CB.RoundMode)
		{
		case ERuneTuningRoundMode::Floor: BaseValue = FMath::FloorToFloat(BaseValue); break;
		case ERuneTuningRoundMode::Round: BaseValue = FMath::RoundToFloat(BaseValue); break;
		case ERuneTuningRoundMode::Ceil:  BaseValue = FMath::CeilToFloat(BaseValue);  break;
		default: break;
		}
	}

	return BaseValue;
}

