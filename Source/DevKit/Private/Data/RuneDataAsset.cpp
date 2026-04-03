#include "Data/RuneDataAsset.h"

FRuneShape FRuneShape::Rotate90() const
{
	FRuneShape Result;

	// 顺时针旋转 90 度（屏幕坐标系，Y 轴向下）
	// 变换公式：(col, row) -> (-row, col)，再归零到左上角
	for (const FIntPoint& Cell : Cells)
	{
		Result.Cells.Add(FIntPoint(-Cell.Y, Cell.X));
	}

	if (Result.Cells.IsEmpty())
		return Result;

	int32 MinX = INT32_MAX;
	int32 MinY = INT32_MAX;
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
	return Instance;
}