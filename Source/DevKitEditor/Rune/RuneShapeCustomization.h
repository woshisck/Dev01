#pragma once

#include "IPropertyTypeCustomization.h"
#include "Math/IntPoint.h"

class IPropertyHandle;

// FRuneShape の Details 패널に格子エディタを埋め込む
class FRuneShapeCustomization : public IPropertyTypeCustomization
{
public:
	static TSharedRef<IPropertyTypeCustomization> MakeInstance()
	{
		return MakeShareable(new FRuneShapeCustomization);
	}

	virtual void CustomizeHeader(
		TSharedRef<IPropertyHandle> InStructPropertyHandle,
		FDetailWidgetRow& HeaderRow,
		IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;

	virtual void CustomizeChildren(
		TSharedRef<IPropertyHandle> InStructPropertyHandle,
		IDetailChildrenBuilder& StructBuilder,
		IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;

private:
	TSharedPtr<IPropertyHandle> StructPropertyHandle;

	// 可视格子半径：显示范围 [-GridHalfSize, GridHalfSize]，即 5x5
	static constexpr int32 GridHalfSize = 2;

	TArray<FIntPoint> GetCurrentCells() const;
	void              SetCells(const TArray<FIntPoint>& NewCells);
	bool              IsCellSelected(int32 X, int32 Y) const;
	void              ToggleCell(int32 X, int32 Y);

	TSharedRef<SWidget> BuildGridWidget();
};