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

    UE_LOG(LogTemp, Warning, TEXT("[GlassFrame] %s NativeConstruct — GlassBG=%s GlassBGCenter=%s GlassBorderImage=%s GlassBorderMaterial=%s"),
        *GetName(),
        GlassBG            ? TEXT("OK") : TEXT("NULL"),
        GlassBGCenter      ? TEXT("OK") : TEXT("NULL"),
        GlassBorderImage   ? TEXT("OK") : TEXT("NULL"),
        GlassBorderMaterial ? *GlassBorderMaterial->GetName() : TEXT("NULL"));

    if (GlassBorderMaterial && GlassBorderImage)
    {
        GlassDynMat = UMaterialInstanceDynamic::Create(GlassBorderMaterial, this);
        GlassBorderImage->SetBrushFromMaterial(GlassDynMat);
        ApplyGlassStyle();
        UE_LOG(LogTemp, Warning, TEXT("[GlassFrame] %s DMI 创建成功"), *GetName());
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[GlassFrame] %s — GlassBorderMaterial 或 GlassBorderImage 未绑定"), *GetName());
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
