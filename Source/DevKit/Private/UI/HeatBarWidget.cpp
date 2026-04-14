#include "UI/HeatBarWidget.h"
#include "Component/BackpackGridComponent.h"
#include "AbilitySystem/Attribute/BaseAttributeSet.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameFramework/Pawn.h"
#include "Components/ProgressBar.h"
#include "Components/Image.h"

// ============================================================
//  颜色常量（sRGB hex → 线性）
//  Phase 0 → BG: 深灰,    Fill: #64FFC0
//  Phase 1 → BG: #64FFC0, Fill: #FFCF72
//  Phase 2+ → BG: #FFCF72, Fill: #FF403D
// ============================================================

namespace HeatBarColors
{
    // #64FFC0FF
    static const FLinearColor Mint   = FLinearColor::FromSRGBColor(FColor(0x64, 0xFF, 0xC0, 0xFF));
    // #FFCF72FF
    static const FLinearColor Gold   = FLinearColor::FromSRGBColor(FColor(0xFF, 0xCF, 0x72, 0xFF));
    // #FF403DFF
    static const FLinearColor Red    = FLinearColor::FromSRGBColor(FColor(0xFF, 0x40, 0x3D, 0xFF));
    // Phase 0 背景深灰
    static const FLinearColor DarkBG = FLinearColor(0.08f, 0.08f, 0.08f, 1.f);
}

// ============================================================
//  内部辅助
// ============================================================

UBackpackGridComponent* UHeatBarWidget::GetBackpack() const
{
    if (CachedBackpack.IsValid()) return CachedBackpack.Get();
    APawn* Pawn = GetOwningPlayerPawn();
    if (!Pawn) return nullptr;
    return Pawn->FindComponentByClass<UBackpackGridComponent>();
}

// ============================================================
//  生命周期
// ============================================================

void UHeatBarWidget::NativeConstruct()
{
    Super::NativeConstruct();

    // 缓存 BackpackGridComponent
    if (APawn* Pawn = GetOwningPlayerPawn())
        CachedBackpack = Pawn->FindComponentByClass<UBackpackGridComponent>();

    if (UBackpackGridComponent* Backpack = GetBackpack())
    {
        Backpack->OnHeatBarUpdate.AddDynamic(this, &UHeatBarWidget::HandleHeatBarUpdate);

        // 立即显示初始状态（从 ASC 读取当前热度）
        float Heat = 0.f, MaxHeat = 1.f;
        if (UAbilitySystemComponent* ASC =
            UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwningPlayerPawn()))
        {
            bool bFound = false;
            Heat    = ASC->GetGameplayAttributeValue(UBaseAttributeSet::GetHeatAttribute(),    bFound);
            MaxHeat = ASC->GetGameplayAttributeValue(UBaseAttributeSet::GetMaxHeatAttribute(), bFound);
        }
        const float NormalizedHeat = (MaxHeat > KINDA_SMALL_NUMBER)
            ? FMath::Clamp(Heat / MaxHeat, 0.f, 1.f) : 0.f;

        RefreshDisplay(NormalizedHeat, Backpack->GetCurrentPhase());
    }
    else
    {
        // 没有背包时显示默认空状态
        RefreshDisplay(0.f, 0);
    }
}

void UHeatBarWidget::NativeDestruct()
{
    if (UBackpackGridComponent* Backpack = GetBackpack())
        Backpack->OnHeatBarUpdate.RemoveDynamic(this, &UHeatBarWidget::HandleHeatBarUpdate);

    Super::NativeDestruct();
}

// ============================================================
//  委托回调
// ============================================================

void UHeatBarWidget::HandleHeatBarUpdate(float NormalizedHeat, int32 NewPhase)
{
    RefreshDisplay(NormalizedHeat, NewPhase);
}

// ============================================================
//  刷新显示
// ============================================================

void UHeatBarWidget::RefreshDisplay(float NormalizedHeat, int32 Phase)
{
    // 根据阶段决定背景色和进度条填充色
    FLinearColor BGColor, FillColor;

    if (Phase <= 0)
    {
        BGColor   = HeatBarColors::DarkBG;
        FillColor = HeatBarColors::Mint;
    }
    else if (Phase == 1)
    {
        BGColor   = HeatBarColors::Mint;
        FillColor = HeatBarColors::Gold;
    }
    else  // Phase 2 及以上（含升华）
    {
        BGColor   = HeatBarColors::Gold;
        FillColor = HeatBarColors::Red;
    }

    if (HeatBarBG)
        HeatBarBG->SetColorAndOpacity(BGColor);

    if (HeatBar)
    {
        HeatBar->SetFillColorAndOpacity(FillColor);
        HeatBar->SetPercent(NormalizedHeat);
    }
}
