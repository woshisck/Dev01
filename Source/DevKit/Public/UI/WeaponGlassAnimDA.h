#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "WeaponGlassAnimDA.generated.h"

/**
 * 武器浮窗→玻璃图标动画配置 DA
 *
 * 流程：WeaponFloat 显示 → [AutoCollapseDelay] →
 *   Phase 1 折叠（隐藏文字/符文区）→
 *   Phase 2 缩小至玻璃图标大小 →
 *   Phase 3 飞向屏幕左下角 →
 *   WeaponGlassIconWidget 常驻显示
 *
 * 开背包时：GlassIcon 放大→渐隐消失
 */
UCLASS(BlueprintType)
class DEVKIT_API UWeaponGlassAnimDA : public UDataAsset
{
	GENERATED_BODY()

public:
	// ─────────────────────────────────────────────
	//  时序
	// ─────────────────────────────────────────────

	/** 浮窗显示后自动折叠延迟（秒）。0 = 不自动，需外部调用 TriggerWeaponCollapse */
	UPROPERTY(EditDefaultsOnly, Category = "时序", meta = (ClampMin = "0"))
	float AutoCollapseDelay = 2.5f;

	// ─────────────────────────────────────────────
	//  Phase 1：折叠（隐藏文字/符文，保留缩略图）
	// ─────────────────────────────────────────────

	UPROPERTY(EditDefaultsOnly, Category = "动画|折叠", meta = (ClampMin = "0"))
	float CollapseDuration = 0.25f;

	// ─────────────────────────────────────────────
	//  Phase 2：缩小至玻璃图标尺寸
	// ─────────────────────────────────────────────

	UPROPERTY(EditDefaultsOnly, Category = "动画|缩小", meta = (ClampMin = "0"))
	float ShrinkDuration = 0.35f;

	/** 飞行终点的玻璃图标尺寸（px，影响缩小目标 Scale） */
	UPROPERTY(EditDefaultsOnly, Category = "动画|缩小")
	FVector2D GlassIconSize = FVector2D(64.f, 64.f);

	// ─────────────────────────────────────────────
	//  Phase 3：飞向 HUD 锚点
	// ─────────────────────────────────────────────

	UPROPERTY(EditDefaultsOnly, Category = "动画|飞行", meta = (ClampMin = "0"))
	float FlyDuration = 0.45f;

	/**
	 * 玻璃图标在屏幕左下角的像素偏移
	 * X = 距离左边缘, Y = 距离底部边缘（正值向上）
	 */
	UPROPERTY(EditDefaultsOnly, Category = "动画|飞行")
	FVector2D HUDOffsetFromBottomLeft = FVector2D(44.f, 120.f);

	// ─────────────────────────────────────────────
	//  开背包：放大→消失
	// ─────────────────────────────────────────────

	UPROPERTY(EditDefaultsOnly, Category = "动画|消失", meta = (ClampMin = "0"))
	float ExpandDuration = 0.20f;

	/** 消失时放大倍率 */
	UPROPERTY(EditDefaultsOnly, Category = "动画|消失",
	          meta = (ClampMin = "1.0", ClampMax = "3.0"))
	float ExpandScale = 1.35f;

	// ─────────────────────────────────────────────
	//  外观
	// ─────────────────────────────────────────────

	/** 飞行过程中缩略图的不透明度（折叠后至抵达 HUD 前） */
	UPROPERTY(EditDefaultsOnly, Category = "外观",
	          meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ThumbnailFlyOpacity = 0.45f;
};
