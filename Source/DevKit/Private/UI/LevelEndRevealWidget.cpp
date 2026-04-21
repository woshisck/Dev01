#include "UI/LevelEndRevealWidget.h"
#include "Components/Image.h"
#include "Materials/MaterialInstanceDynamic.h"

UMaterialInstanceDynamic* ULevelEndRevealWidget::InitReveal(UMaterialInterface* Mat,
                                                             FVector2D LootScreenUV,
                                                             float EdgeSharpness)
{
	if (!RevealImage || !Mat) return nullptr;

	UMaterialInstanceDynamic* DynMat = UMaterialInstanceDynamic::Create(Mat, this);
	RevealImage->SetBrushFromMaterial(DynMat);

	DynMat->SetVectorParameterValue(TEXT("RevealCenter"),
		FLinearColor(LootScreenUV.X, LootScreenUV.Y, 0.f, 0.f));
	DynMat->SetScalarParameterValue(TEXT("EdgeSharpness"), EdgeSharpness);
	DynMat->SetScalarParameterValue(TEXT("RevealProgress"), 0.f);

	return DynMat;
}
