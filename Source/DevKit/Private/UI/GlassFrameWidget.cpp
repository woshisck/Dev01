#include "UI/GlassFrameWidget.h"
#include "Components/BackgroundBlur.h"
#include "Components/Image.h"
#include "Materials/MaterialInstanceDynamic.h"

void UGlassFrameWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (GlassBG)
        GlassBG->SetBlurStrength(BlurStrength);

    if (GlassBGCenter)
        GlassBGCenter->SetBlurStrength(CenterBlurStrength);

    if (GlassBorderMaterial && GlassBorderImage)
    {
        GlassDynMat = UMaterialInstanceDynamic::Create(GlassBorderMaterial, this);
        GlassBorderImage->SetBrushFromMaterial(GlassDynMat);
        ApplyGlassStyle();
    }
}

void UGlassFrameWidget::ApplyGlassStyle()
{
    if (GlassBG)
        GlassBG->SetBlurStrength(BlurStrength);

    if (GlassBGCenter)
        GlassBGCenter->SetBlurStrength(CenterBlurStrength);

    if (!GlassDynMat) return;

    GlassDynMat->SetScalarParameterValue(TEXT("CornerRadius"),  CornerRadius);
    GlassDynMat->SetScalarParameterValue(TEXT("BorderWidth"),   BorderWidth);
    GlassDynMat->SetScalarParameterValue(TEXT("FresnelPower"),  FresnelPower);
    GlassDynMat->SetScalarParameterValue(TEXT("IridIntensity"), IridIntensity);
    GlassDynMat->SetScalarParameterValue(TEXT("IridSpeed"),     IridSpeed);
}
