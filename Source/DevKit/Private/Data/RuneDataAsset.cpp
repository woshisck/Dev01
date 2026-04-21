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

