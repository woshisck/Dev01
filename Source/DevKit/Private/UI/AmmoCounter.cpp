// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/AmmoCounter.h"
#include "AbilitySystem/Attribute/PlayerAttributeSet.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Styling/SlateBrush.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

void UAmmoCounter::NativeConstruct()
{
    Super::NativeConstruct();
    BindToASC();
}

void UAmmoCounter::NativeDestruct()
{
    UnbindFromASC();
    Super::NativeDestruct();
}

void UAmmoCounter::BindToASC()
{
    APlayerController* PC = GetOwningPlayer();
    if (!PC)
    {
        UE_LOG(LogTemp, Warning, TEXT("[AmmoCounter] BindToASC: No PlayerController"));
        return;
    }

    APawn* Pawn = PC->GetPawn();
    if (!Pawn)
    {
        UE_LOG(LogTemp, Warning, TEXT("[AmmoCounter] BindToASC: No Pawn on PlayerController"));
        return;
    }

    UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Pawn);
    if (!ASC)
    {
        UE_LOG(LogTemp, Warning, TEXT("[AmmoCounter] BindToASC: No ASC on Pawn %s"), *Pawn->GetName());
        return;
    }

    // 获取初始值
    bool bFoundCurrent = false, bFoundMax = false;
    CachedCurrent = FMath::RoundToInt(
        ASC->GetGameplayAttributeValue(UPlayerAttributeSet::GetCurrentAmmoAttribute(), bFoundCurrent));
    CachedMax = FMath::RoundToInt(
        ASC->GetGameplayAttributeValue(UPlayerAttributeSet::GetMaxAmmoAttribute(), bFoundMax));

    UE_LOG(LogTemp, Warning, TEXT("[AmmoCounter] BindToASC OK — CurrentAmmo=%d(found=%d) MaxAmmo=%d(found=%d) BulletIconBox=%s"),
        CachedCurrent, bFoundCurrent, CachedMax, bFoundMax,
        BulletIconBox ? TEXT("OK") : TEXT("NULL"));

    // 绑定属性变化委托
    CurrentAmmoHandle = ASC->GetGameplayAttributeValueChangeDelegate(
        UPlayerAttributeSet::GetCurrentAmmoAttribute())
        .AddUObject(this, &UAmmoCounter::OnCurrentAmmoChanged);

    MaxAmmoHandle = ASC->GetGameplayAttributeValueChangeDelegate(
        UPlayerAttributeSet::GetMaxAmmoAttribute())
        .AddUObject(this, &UAmmoCounter::OnMaxAmmoChanged);

    RebuildIcons(CachedMax);
    RefreshIconColors(CachedCurrent, CachedMax);
}

void UAmmoCounter::UnbindFromASC()
{
    APlayerController* PC = GetOwningPlayer();
    if (!PC) return;

    APawn* Pawn = PC->GetPawn();
    if (!Pawn) return;

    UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Pawn);
    if (!ASC) return;

    if (CurrentAmmoHandle.IsValid())
        ASC->GetGameplayAttributeValueChangeDelegate(
            UPlayerAttributeSet::GetCurrentAmmoAttribute()).Remove(CurrentAmmoHandle);

    if (MaxAmmoHandle.IsValid())
        ASC->GetGameplayAttributeValueChangeDelegate(
            UPlayerAttributeSet::GetMaxAmmoAttribute()).Remove(MaxAmmoHandle);
}

void UAmmoCounter::OnCurrentAmmoChanged(const FOnAttributeChangeData& Data)
{
    CachedCurrent = FMath::RoundToInt(Data.NewValue);
    RefreshIconColors(CachedCurrent, CachedMax);
}

void UAmmoCounter::OnMaxAmmoChanged(const FOnAttributeChangeData& Data)
{
    CachedMax = FMath::RoundToInt(Data.NewValue);
    RebuildIcons(CachedMax);
    RefreshIconColors(CachedCurrent, CachedMax);
}

void UAmmoCounter::RebuildIcons(int32 Max)
{
    UE_LOG(LogTemp, Warning, TEXT("[AmmoCounter] RebuildIcons: Max=%d BulletIconBox=%s"),
        Max, BulletIconBox ? TEXT("OK") : TEXT("NULL"));
    if (!BulletIconBox) return;

    BulletIconBox->ClearChildren();
    BulletIcons.Reset();

    // 纯色 Brush（无纹理，用 Tint 区分有弹/空仓）
    FSlateBrush Brush;
    Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
    Brush.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
    Brush.OutlineSettings.CornerRadii  = FVector4(3.f, 3.f, 3.f, 3.f);

    for (int32 i = 0; i < Max; ++i)
    {
        UImage* Icon = NewObject<UImage>(this);
        Icon->SetBrush(Brush);
        Icon->SetColorAndOpacity(FilledColor);

        UHorizontalBoxSlot* IconSlot = BulletIconBox->AddChildToHorizontalBox(Icon);
        if (IconSlot)
        {
            IconSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
            IconSlot->SetPadding(FMargin(i == 0 ? 0.f : IconPadding, 0.f, 0.f, 0.f));
        }

        // 手动设置图标大小
        Icon->SetDesiredSizeOverride(IconSize);

        BulletIcons.Add(Icon);
    }
}

void UAmmoCounter::RefreshIconColors(int32 Current, int32 Max)
{
    for (int32 i = 0; i < BulletIcons.Num(); ++i)
    {
        if (BulletIcons[i])
        {
            BulletIcons[i]->SetColorAndOpacity(i < Current ? FilledColor : EmptyColor);
        }
    }
}
