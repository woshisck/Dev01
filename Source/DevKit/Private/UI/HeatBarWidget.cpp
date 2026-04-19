#include "UI/HeatBarWidget.h"
#include "Component/BackpackGridComponent.h"
#include "AbilitySystem/Attribute/BaseAttributeSet.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameFramework/Pawn.h"
#include "Components/ProgressBar.h"
#include "Components/Image.h"
#include "Character/PlayerCharacterBase.h"
#include "Kismet/GameplayStatics.h"

// ============================================================
//  颜色常量（sRGB hex → 线性）
//  Phase 1: 冷白/淡蓝 #7BA8FF
//  Phase 2: 暖橙      #FF8020
//  Phase 3: 金色      #FFD020
// ============================================================

namespace HeatBarColors
{
    // Phase 1: 冷白/淡蓝 #7BA8FF
    static const FLinearColor CoolBlue   = FLinearColor::FromSRGBColor(FColor(0x7B, 0xA8, 0xFF, 0xFF));
    // Phase 2: 暖橙 #FF8020
    static const FLinearColor WarmOrange = FLinearColor::FromSRGBColor(FColor(0xFF, 0x80, 0x20, 0xFF));
    // Phase 3: 金色 #FFD020
    static const FLinearColor Gold       = FLinearColor::FromSRGBColor(FColor(0xFF, 0xD0, 0x20, 0xFF));
    // Phase 0 背景深灰
    static const FLinearColor DarkBG     = FLinearColor(0.08f, 0.08f, 0.08f, 1.f);
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

    if (APlayerCharacterBase* player = Cast<APlayerCharacterBase>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0)))
    {
        CachedBackpack = player->GetBackpackGridComponent();
    }

    if (APlayerCharacterBase* player = Cast<APlayerCharacterBase>(GetOwningPlayerPawn()))
        CachedBackpack = player->FindComponentByClass<UBackpackGridComponent>();

    if (CachedBackpack.IsValid())
    {
        CachedBackpack->OnHeatBarUpdate.AddDynamic(this, &UHeatBarWidget::HandleHeatBarUpdate);

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
        RefreshDisplay(NormalizedHeat, CachedBackpack->GetCurrentPhase());
    }
    else
    {
        RefreshDisplay(0.f, 0);
    }
}

void UHeatBarWidget::NativeDestruct()
{
    if (UBackpackGridComponent* Backpack = GetBackpack())
        Backpack->OnHeatBarUpdate.RemoveDynamic(this, &UHeatBarWidget::HandleHeatBarUpdate_Implementation);

    Super::NativeDestruct();
}

// ============================================================
//  委托回调
// ============================================================

void UHeatBarWidget::HandleHeatBarUpdate_Implementation(float NormalizedHeat, int32 NewPhase)
{
    const int32 PhaseBefore = CurrentPhase;
    const int32 PhaseNext   = NewPhase;

    if (PhaseBefore != PhaseNext)
    {
        CurrentPhase = PhaseNext;
    }

    RefreshDisplay(NormalizedHeat, CurrentPhase);
}

// ============================================================
//  刷新显示
//  BG 色 = 当前阶段颜色（已达到的底色）
//  Fill 色 = 下一阶段颜色（正在积累的进度）
// ============================================================

void UHeatBarWidget::RefreshDisplay(float NormalizedHeat, int32 Phase)
{
    FLinearColor BGColor, FillColor;

    if (Phase <= 0)
    {
        BGColor   = HeatBarColors::DarkBG;
        FillColor = HeatBarColors::CoolBlue;    // 向 Phase1 冲
    }
    else if (Phase == 1)
    {
        BGColor   = HeatBarColors::CoolBlue;    // Phase1 已达
        FillColor = HeatBarColors::WarmOrange;  // 向 Phase2 冲
    }
    else  // Phase 2+
    {
        BGColor   = HeatBarColors::WarmOrange;  // Phase2 已达
        FillColor = HeatBarColors::Gold;        // 向 Phase3 冲
    }

    if (HeatBarBG)
        HeatBarBG->SetColorAndOpacity(BGColor);

    if (HeatBar)
    {
        HeatBar->SetFillColorAndOpacity(FillColor);
        HeatBar->SetPercent(NormalizedHeat);
    }
}
