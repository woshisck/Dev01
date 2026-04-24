#include "DevKitEditor/Rune/RuneShapeCustomization.h"
#include "Data/RuneDataAsset.h"

#include "DetailWidgetRow.h"
#include "IDetailChildrenBuilder.h"
#include "PropertyHandle.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "Styling/AppStyle.h"

#define LOCTEXT_NAMESPACE "RuneShapeCustomization"

// ──────────────────────────────────────────────────────────────
//  Header：标题行只显示属性名 + 简短说明
// ──────────────────────────────────────────────────────────────

void FRuneShapeCustomization::CustomizeHeader(
	TSharedRef<IPropertyHandle> InStructPropertyHandle,
	FDetailWidgetRow& HeaderRow,
	IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	StructPropertyHandle = InStructPropertyHandle;

	HeaderRow
		.NameContent()
		[
			InStructPropertyHandle->CreatePropertyNameWidget()
		]
		.ValueContent()
		[
			SNew(STextBlock)
			.Text(LOCTEXT("ShapeHint", "点击格子切换占用"))
			.Font(FAppStyle::GetFontStyle(TEXT("PropertyWindow.NormalFont")))
		];
}

// ──────────────────────────────────────────────────────────────
//  Children：插入可视化格子编辑器
// ──────────────────────────────────────────────────────────────

void FRuneShapeCustomization::CustomizeChildren(
	TSharedRef<IPropertyHandle> InStructPropertyHandle,
	IDetailChildrenBuilder& StructBuilder,
	IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	StructBuilder.AddCustomRow(LOCTEXT("RuneShapeGrid", "形状编辑器"))
	[
		SNew(SBox)
		.Padding(FMargin(4.f, 4.f))
		[
			BuildGridWidget()
		]
	];
}

// ──────────────────────────────────────────────────────────────
//  构建 5×5 点击格子
//    列→X，行→Y（上行=正Y，下行=负Y）
//    (0,0) 位于中心格，作为符文 Pivot
// ──────────────────────────────────────────────────────────────

TSharedRef<SWidget> FRuneShapeCustomization::BuildGridWidget()
{
	constexpr int32 Size = GridHalfSize * 2 + 1; // 5

	TSharedRef<SUniformGridPanel> Grid = SNew(SUniformGridPanel)
		.SlotPadding(FMargin(2.f));

	for (int32 Row = 0; Row < Size; ++Row)
	{
		for (int32 Col = 0; Col < Size; ++Col)
		{
			const int32 X = Col - GridHalfSize;         // 左负右正
			const int32 Y = GridHalfSize - Row;         // 上正下负

			const bool bIsOrigin = (X == 0 && Y == 0);

			Grid->AddSlot(Col, Row)
			[
				SNew(SBox)
				.WidthOverride(28.f)
				.HeightOverride(28.f)
				[
					SNew(SButton)
					.ButtonStyle(FAppStyle::Get(), "FlatButton.Dark")
					// 选中=蓝，原点=稍浅灰，普通=深灰
					.ButtonColorAndOpacity_Lambda([this, X, Y, bIsOrigin]() -> FSlateColor
					{
						if (IsCellSelected(X, Y))
							return FLinearColor(0.1f, 0.45f, 0.95f, 1.f);
						return bIsOrigin
							? FLinearColor(0.22f, 0.22f, 0.22f, 1.f)
							: FLinearColor(0.10f, 0.10f, 0.10f, 1.f);
					})
					.ContentPadding(FMargin(0))
					.OnClicked_Lambda([this, X, Y]() -> FReply
					{
						ToggleCell(X, Y);
						return FReply::Handled();
					})
					.ToolTipText(FText::Format(
						LOCTEXT("CellTooltip", "({0}, {1})\n点击切换"),
						FText::AsNumber(X), FText::AsNumber(Y)
					))
				]
			];
		}
	}

	return Grid;
}

// ──────────────────────────────────────────────────────────────
//  读写 FRuneShape::Cells（直接访问原始内存，支持多选同步）
// ──────────────────────────────────────────────────────────────

TArray<FIntPoint> FRuneShapeCustomization::GetCurrentCells() const
{
	TArray<void*> RawData;
	StructPropertyHandle->AccessRawData(RawData);

	if (RawData.IsEmpty() || !RawData[0])
		return {};

	return reinterpret_cast<const FRuneShape*>(RawData[0])->Cells;
}

void FRuneShapeCustomization::SetCells(const TArray<FIntPoint>& NewCells)
{
	StructPropertyHandle->NotifyPreChange();

	TArray<void*> RawData;
	StructPropertyHandle->AccessRawData(RawData);

	for (void* Data : RawData)
	{
		if (FRuneShape* Shape = reinterpret_cast<FRuneShape*>(Data))
			Shape->Cells = NewCells;
	}

	StructPropertyHandle->NotifyPostChange(EPropertyChangeType::ValueSet);
}

bool FRuneShapeCustomization::IsCellSelected(int32 X, int32 Y) const
{
	return GetCurrentCells().Contains(FIntPoint(X, Y));
}

void FRuneShapeCustomization::ToggleCell(int32 X, int32 Y)
{
	TArray<FIntPoint> Cells = GetCurrentCells();
	const FIntPoint Point(X, Y);

	if (Cells.Contains(Point))
		Cells.Remove(Point);
	else
		Cells.Add(Point);

	SetCells(Cells);
}

#undef LOCTEXT_NAMESPACE
