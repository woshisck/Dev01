#include "UI/LiquidHealthBarWidget.h"
#include "Components/Image.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

namespace
{
    constexpr TCHAR DefaultLiquidHealthBarMaterialPath[] = TEXT("/Game/UI/UI_Material/HUD/M_LiquidHealthBar.M_LiquidHealthBar");
    constexpr TCHAR DefaultLiquidDripMaterialPath[] = TEXT("/Game/UI/UI_Material/HUD/M_LiquidHealthBarDrips.M_LiquidHealthBarDrips");
}

void ULiquidHealthBarWidget::NativeConstruct()
{
    Super::NativeConstruct();

    EnsureDynamicMaterial();
    EnsureDripMaterial();
    UpdateLeakParams();
}

void ULiquidHealthBarWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    if (!bNeedsTick || !LiquidDynMat) return;

    // 推进振荡相位（持续累加，不归零，保证波形连续）
    SloshPh  += InDeltaTime * OscFrequency;

    // 指数阻尼：exp(-k*t) 模拟粘稠液体摩擦力
    SloshAmp *= FMath::Exp(-DampingRate * InDeltaTime);

    LiquidDynMat->SetScalarParameterValue(TEXT("SloshAmplitude"), SloshAmp);
    LiquidDynMat->SetScalarParameterValue(TEXT("SloshPhase"),     SloshPh);

    // 幅度低于感知阈值时停止 Tick
    if (SloshAmp < 0.0004f)
    {
        SloshAmp   = 0.f;
        bNeedsTick = false;
        LiquidDynMat->SetScalarParameterValue(TEXT("SloshAmplitude"), 0.f);
    }
}

void ULiquidHealthBarWidget::SetHealthPercent(float NewPct)
{
    NewPct = FMath::Clamp(NewPct, 0.f, 1.f);

    const float OldPct = CurrentPct;
    const float Delta = FMath::Abs(NewPct - CurrentPct);
    CurrentPct = NewPct;

    if (EnsureDynamicMaterial())
    {
        const float FillValue = CurrentPct * FillWindowEnd;
        LiquidDynMat->SetScalarParameterValue(TEXT("FillPercent"), FillValue);
        UE_LOG(LogTemp, Warning, TEXT("[LiquidHB] SetHealthPercent %.3f -> %.3f | FillPercent=%.3f FillWindowEnd=%.3f"),
            OldPct, CurrentPct, FillValue, FillWindowEnd);

        if (Delta > KINDA_SMALL_NUMBER)
        {
            // 血量变化越大，初始晃动幅度越大；但不超过 MaxSloshAmplitude
            // 系数 0.22：Delta=1.0 时产生约 80% 最大幅度，感知直觉较强
            SloshAmp   = FMath::Clamp(Delta * 0.22f, 0.008f, MaxSloshAmplitude);
            bNeedsTick = true;
        }
    }

    // Leak tracks health independently of the liquid layer, so it still updates
    // even when the tube material is missing or misconfigured.
    EnsureDripMaterial();
    UpdateLeakParams();
}

float ULiquidHealthBarWidget::GetLeakIntensity() const
{
    if (LeakThreshold <= KINDA_SMALL_NUMBER)
    {
        return 0.f;
    }

    const float Ramp = FMath::Clamp((LeakThreshold - CurrentPct) / LeakThreshold, 0.f, 1.f);
    return FMath::Pow(Ramp, LeakCurveExponent);
}

void ULiquidHealthBarWidget::UpdateLeakParams()
{
    if (!DripDynMat)
    {
        return;
    }

    DripDynMat->SetScalarParameterValue(TEXT("LeakIntensity"), GetLeakIntensity());

    // Droplets originate at the liquid's leading edge, so the leak follows the level.
    DripDynMat->SetScalarParameterValue(TEXT("LeakX"), CurrentPct * FillWindowEnd);
}

void ULiquidHealthBarWidget::ApplyColors()
{
    if (LiquidDynMat)
    {
        LiquidDynMat->SetVectorParameterValue(TEXT("LiquidColorDeep"),    LiquidColorDeep);
        LiquidDynMat->SetVectorParameterValue(TEXT("LiquidColorSurface"), LiquidColorSurface);
        LiquidDynMat->SetVectorParameterValue(TEXT("GlintColor"),         GlintColor);
    }

    if (DripDynMat)
    {
        DripDynMat->SetVectorParameterValue(TEXT("DripColor"),  DripColor);
        DripDynMat->SetVectorParameterValue(TEXT("StainColor"), StainColor);
    }
}

bool ULiquidHealthBarWidget::EnsureDynamicMaterial()
{
    if (LiquidDynMat)
    {
        return true;
    }

    if (!LiquidFillImage)
    {
        if (!bLoggedMissingMaterialSetup)
        {
            UE_LOG(LogTemp, Warning, TEXT("[LiquidHB] LiquidFillImage is not bound; health percent will be cached until the widget is configured."));
            bLoggedMissingMaterialSetup = true;
        }
        return false;
    }

    UMaterialInterface* SourceMaterial = LiquidMaterial;
    if (!SourceMaterial)
    {
        SourceMaterial = Cast<UMaterialInterface>(LiquidFillImage->GetBrush().GetResourceObject());
    }

    if (!SourceMaterial)
    {
        SourceMaterial = LoadObject<UMaterialInterface>(nullptr, DefaultLiquidHealthBarMaterialPath);
        if (SourceMaterial)
        {
            LiquidMaterial = SourceMaterial;
        }
    }

    if (!SourceMaterial)
    {
        if (!bLoggedMissingMaterialSetup)
        {
            UE_LOG(LogTemp, Warning, TEXT("[LiquidHB] LiquidMaterial is not set and fallback material '%s' could not be loaded."), DefaultLiquidHealthBarMaterialPath);
            bLoggedMissingMaterialSetup = true;
        }
        return false;
    }

    LiquidDynMat = UMaterialInstanceDynamic::Create(SourceMaterial, this);
    if (!LiquidDynMat)
    {
        if (!bLoggedMissingMaterialSetup)
        {
            UE_LOG(LogTemp, Warning, TEXT("[LiquidHB] Failed to create dynamic material instance from '%s'."), *SourceMaterial->GetName());
            bLoggedMissingMaterialSetup = true;
        }
        return false;
    }

    LiquidFillImage->SetBrushFromMaterial(LiquidDynMat);
    LiquidDynMat->SetScalarParameterValue(TEXT("FillPercent"), CurrentPct * FillWindowEnd);
    LiquidDynMat->SetScalarParameterValue(TEXT("SloshAmplitude"), 0.f);
    LiquidDynMat->SetScalarParameterValue(TEXT("SloshPhase"), 0.f);
    ApplyColors();

    return true;
}

bool ULiquidHealthBarWidget::EnsureDripMaterial()
{
    if (DripDynMat)
    {
        return true;
    }

    // The leak layer is opt-in: no DripImage in the WBP means no leak, not an error.
    if (!DripImage)
    {
        return false;
    }

    UMaterialInterface* SourceMaterial = DripMaterial;
    if (!SourceMaterial)
    {
        SourceMaterial = Cast<UMaterialInterface>(DripImage->GetBrush().GetResourceObject());
    }

    if (!SourceMaterial && !bTriedDripFallbackLoad)
    {
        bTriedDripFallbackLoad = true;
        SourceMaterial = LoadObject<UMaterialInterface>(nullptr, DefaultLiquidDripMaterialPath);
        if (SourceMaterial)
        {
            DripMaterial = SourceMaterial;
        }
    }

    if (!SourceMaterial)
    {
        return false;
    }

    DripDynMat = UMaterialInstanceDynamic::Create(SourceMaterial, this);
    if (!DripDynMat)
    {
        return false;
    }

    DripImage->SetBrushFromMaterial(DripDynMat);
    ApplyColors();
    UpdateLeakParams();

    return true;
}
