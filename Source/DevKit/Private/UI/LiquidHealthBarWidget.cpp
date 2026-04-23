#include "UI/LiquidHealthBarWidget.h"
#include "Components/Image.h"
#include "Materials/MaterialInstanceDynamic.h"

void ULiquidHealthBarWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (LiquidMaterial && LiquidFillImage)
    {
        LiquidDynMat = UMaterialInstanceDynamic::Create(LiquidMaterial, this);
        LiquidFillImage->SetBrushFromMaterial(LiquidDynMat);

        LiquidDynMat->SetScalarParameterValue(TEXT("FillPercent"),    CurrentPct * FillWindowEnd);
        LiquidDynMat->SetScalarParameterValue(TEXT("SloshAmplitude"), 0.f);
        LiquidDynMat->SetScalarParameterValue(TEXT("SloshPhase"),     0.f);
        ApplyColors();
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[LiquidHB] DMI 未创建 — LiquidMaterial 或 LiquidFillImage 未绑定"));
    }
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

    const float Delta = FMath::Abs(NewPct - CurrentPct);
    CurrentPct = NewPct;

    if (!LiquidDynMat)
    {
        UE_LOG(LogTemp, Error, TEXT("[LiquidHB] SetHealthPercent(%.2f) — LiquidDynMat 为 null，跳过"), NewPct);
        return;
    }

    LiquidDynMat->SetScalarParameterValue(TEXT("FillPercent"), CurrentPct * FillWindowEnd);

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
