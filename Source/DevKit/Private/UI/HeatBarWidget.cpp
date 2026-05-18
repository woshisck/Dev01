#include "UI/HeatBarWidget.h"
#include "AbilitySystem/Attribute/BaseAttributeSet.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameFramework/Pawn.h"
#include "Components/ProgressBar.h"
#include "Components/Image.h"
#include "Character/PlayerCharacterBase.h"
#include "UI/BackpackStyleDataAsset.h"

// 兜底颜色（DA 未配置时使用）
namespace HeatBarColors
{
    static const FLinearColor CoolBlue   = FLinearColor::FromSRGBColor(FColor(0x7B, 0xA8, 0xFF, 0xFF));
    static const FLinearColor WarmOrange = FLinearColor::FromSRGBColor(FColor(0xFF, 0x80, 0x20, 0xFF));
    static const FLinearColor Gold       = FLinearColor::FromSRGBColor(FColor(0xFF, 0xD0, 0x20, 0xFF));
    static const FLinearColor DarkBG     = FLinearColor(0.08f, 0.08f, 0.08f, 1.f);
}

// ============================================================
//  生命周期
// ============================================================

void UHeatBarWidget::NativeConstruct()
{
    Super::NativeConstruct();

    float Heat = 0.f, MaxHeat = 1.f;
    if (UAbilitySystemComponent* ASC =
        UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwningPlayerPawn()))
    {
        bool bFound = false;
        Heat    = ASC->GetGameplayAttributeValue(UBaseAttributeSet::GetHeatAttribute(), bFound);
        MaxHeat = ASC->GetGameplayAttributeValue(UBaseAttributeSet::GetMaxHeatAttribute(), bFound);
    }
    const float NormalizedHeat = (MaxHeat > KINDA_SMALL_NUMBER)
        ? FMath::Clamp(Heat / MaxHeat, 0.f, 1.f) : 0.f;
    OnHeatBarUpdateReceived(NormalizedHeat, 0);
}

void UHeatBarWidget::NativeDestruct()
{
    Super::NativeDestruct();
}

// ============================================================
//  委托回调
// ============================================================

void UHeatBarWidget::OnHeatBarUpdateReceived(float NormalizedHeat, int32 NewPhase)
{
    PreviousPhase = CurrentPhase;
    bPhaseChanged = (PreviousPhase != NewPhase);
    CurrentPhase = NewPhase;

    HandleHeatBarUpdate(NormalizedHeat, NewPhase);
}

void UHeatBarWidget::HandleHeatBarUpdate_Implementation(float NormalizedHeat, int32 NewPhase)
{
    RefreshDisplay(NormalizedHeat, NewPhase);
}

// ============================================================
//  刷新显示
//  BG 色 = 当前阶段颜色（已达到的底色）
//  Fill 色 = 下一阶段颜色（正在积累的进度）
// ============================================================

void UHeatBarWidget::RefreshDisplay(float NormalizedHeat, int32 Phase)
{
    // 从玩家角色的 HeatStyleDA 读取颜色，兜底走硬编码常量
    FLinearColor C0 = HeatBarColors::CoolBlue;
    FLinearColor C1 = HeatBarColors::WarmOrange;
    FLinearColor C2 = HeatBarColors::Gold;

    APlayerCharacterBase* Char = Cast<APlayerCharacterBase>(GetOwningPlayerPawn());

    if (Char)
    {
        if (UBackpackStyleDataAsset* DA = Char->HeatStyleDA)
        {
            C0 = DA->HeatZone0Color;
            C1 = DA->HeatZone1Color;
            C2 = DA->HeatZone2Color;
        }
    }

    FLinearColor BGColor, FillColor;

    if (Phase <= 0)
    {
        BGColor   = HeatBarColors::DarkBG;
        FillColor = C0;
    }
    else if (Phase == 1)
    {
        BGColor   = C0;
        FillColor = C1;
    }
    else  // Phase 2+
    {
        BGColor   = C1;
        FillColor = C2;
    }

    if (HeatBarBG)
        HeatBarBG->SetColorAndOpacity(BGColor);

    if (HeatBar)
    {
        HeatBar->SetFillColorAndOpacity(FillColor);
        HeatBar->SetPercent(NormalizedHeat);
    }
}
