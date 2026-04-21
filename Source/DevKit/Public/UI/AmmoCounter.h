// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayEffectTypes.h"
#include "AmmoCounter.generated.h"

class UHorizontalBox;
class UImage;

/**
 * 弹药计数 HUD Widget。
 *
 * Blueprint 子类只需在 Designer 中放置一个 HorizontalBox，命名为 BulletIconBox，
 * 无需编写任何蓝图逻辑——所有功能由 C++ 驱动。
 *
 * 无纹理资产时以纯色矩形显示：
 *   有弹 → 金色  (R=1.0, G=0.8, B=0.1)
 *   空仓 → 灰色  (R=0.2, G=0.2, B=0.2)
 */
UCLASS()
class DEVKIT_API UAmmoCounter : public UUserWidget
{
    GENERATED_BODY()

public:
    /** 子弹图标显示容器（Blueprint Designer 中命名 BulletIconBox） */
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UHorizontalBox> BulletIconBox;

    /** 图标尺寸（px），Blueprint Class Defaults 可调 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AmmoCounter")
    FVector2D IconSize = FVector2D(22.f, 22.f);

    /** 图标间距 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AmmoCounter")
    float IconPadding = 4.f;

    /** 有弹颜色 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AmmoCounter")
    FLinearColor FilledColor = FLinearColor(1.f, 0.8f, 0.1f, 1.f);

    /** 空仓颜色 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AmmoCounter")
    FLinearColor EmptyColor = FLinearColor(0.2f, 0.2f, 0.2f, 0.6f);

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

private:
    void BindToASC();
    void UnbindFromASC();

    void OnCurrentAmmoChanged(const FOnAttributeChangeData& Data);
    void OnMaxAmmoChanged(const FOnAttributeChangeData& Data);

    /** 根据 Current/MaxAmmo 重建或刷新图标列表 */
    void RebuildIcons(int32 Max);
    void RefreshIconColors(int32 Current, int32 Max);

    FDelegateHandle CurrentAmmoHandle;
    FDelegateHandle MaxAmmoHandle;

    int32 CachedMax     = 0;
    int32 CachedCurrent = 0;

    UPROPERTY()
    TArray<TObjectPtr<UImage>> BulletIcons;
};
