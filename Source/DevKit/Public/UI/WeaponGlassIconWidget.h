#pragma once

#include "CoreMinimal.h"
#include "UI/GlassFrameWidget.h"
#include "WeaponGlassIconWidget.generated.h"

class UImage;
class UWeaponGlassAnimDA;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnWeaponGlassHidden);

/**
 * 武器玻璃图标 — 拾取武器后常驻于屏幕左下角的液态玻璃方块
 *
 * WBP 层级（父类 WeaponGlassIconWidget）：
 *   [Root] Overlay（Fill）
 *   ├── BackgroundBlur  "GlassBG"          ← 毛玻璃底（继承自 GlassFrameWidget）
 *   ├── Image           "GlassBorderImage"  ← 玻璃边框材质（继承自 GlassFrameWidget）
 *   ├── Image           "WeaponThumbnailImg" ← 武器缩略图（半透明）
 *   └── Image           "HeatColorOverlay"  ← 热度颜色叠加（乘法/Additive 混合）
 *
 * 调用顺序：
 *   YogHUD 在武器飞行结束后调用 ShowForWeapon → 开背包时调用 StartExpandAndHide
 */
UCLASS(Blueprintable, BlueprintType)
class DEVKIT_API UWeaponGlassIconWidget : public UGlassFrameWidget
{
	GENERATED_BODY()

public:
	/** 飞行结束后调用：显示武器缩略图，进入常驻状态 */
	UFUNCTION(BlueprintCallable, Category = "WeaponGlass")
	void ShowForWeapon(UTexture2D* Thumbnail, const UWeaponGlassAnimDA* InAnimDA);

	/** 同步热度颜色（与热度槽颜色一致） */
	UFUNCTION(BlueprintCallable, Category = "WeaponGlass")
	void SetHeatColor(FLinearColor Color);

	/** 触发放大→渐隐消失动画（开背包时调用） */
	UFUNCTION(BlueprintCallable, Category = "WeaponGlass")
	void StartExpandAndHide();

	/** 消失动画播完后广播 */
	UPROPERTY(BlueprintAssignable, Category = "WeaponGlass")
	FOnWeaponGlassHidden OnHidden;

protected:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UImage> WeaponThumbnailImg;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UImage> HeatColorOverlay;

private:
	UPROPERTY()
	TObjectPtr<const UWeaponGlassAnimDA> AnimDA;

	bool  bExpanding    = false;
	float ExpandTimer   = 0.f;
};
