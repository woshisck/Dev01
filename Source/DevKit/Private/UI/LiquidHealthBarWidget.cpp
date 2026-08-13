#include "UI/LiquidHealthBarWidget.h"
#include "Components/Image.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

namespace
{
    constexpr TCHAR DefaultLiquidHealthBarMaterialPath[] = TEXT("/Game/UI/UI_Material/HUD/M_LiquidHealthBar.M_LiquidHealthBar");
}

void ULiquidHealthBarWidget::NativeConstruct()
{
    Super::NativeConstruct();

    EnsureDynamicMaterial();
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

    if (!EnsureDynamicMaterial())
    {
        return;
    }

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

void ULiquidHealthBarWidget::ApplyColors()
{
    if (!LiquidDynMat) return;

    LiquidDynMat->SetVectorParameterValue(TEXT("LiquidColorDeep"),    LiquidColorDeep);
    LiquidDynMat->SetVectorParameterValue(TEXT("LiquidColorSurface"), LiquidColorSurface);
    LiquidDynMat->SetVectorParameterValue(TEXT("GlintColor"),         GlintColor);
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
