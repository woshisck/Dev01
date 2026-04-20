#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "WeaponThumbnailFlyWidget.generated.h"

class UImage;
class UWeaponGlassAnimDA;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnThumbnailFlyComplete, UTexture2D*, Thumbnail);

/** Flying 阶段每帧广播 (起点, 当前位置, 进度 0-1)，供 WeaponTrailWidget 绘制流光 */
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnThumbnailFlyProgress, FVector2D, FVector2D, float);

/**
 * 武器缩略图飞行 Widget — 仅包含一张 Image，C++ 驱动屏幕坐标平移。
 *
 * WBP 结构：根节点 CanvasPanel（全屏 HitTestInvisible），内含 Image 命名 "ThumbnailImage"
 */
UCLASS()
class DEVKIT_API UWeaponThumbnailFlyWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/**
	 * 开始飞行动画（AddToViewport 后调用）
	 * @param Thumbnail    武器缩略图
	 * @param StartAbsPos  起点屏幕绝对坐标（中心）
	 * @param EndAbsPos    终点屏幕绝对坐标（GlassIcon 中心）
	 * @param DA           动画参数（FlyDuration / GlassIconSize / ThumbnailFlyOpacity）
	 */
	void StartFly(UTexture2D* Thumbnail, FVector2D StartAbsPos, FVector2D EndAbsPos,
	              const UWeaponGlassAnimDA* DA);

	/** 飞行完成时广播，参数为缩略图贴图（供 GlassIconWidget 使用） */
	UPROPERTY(BlueprintAssignable, Category = "ThumbnailFly")
	FOnThumbnailFlyComplete OnFlyComplete;

	/** 每帧广播，供 WeaponTrailWidget 更新流光线段 */
	FOnThumbnailFlyProgress OnFlyProgress;

protected:
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> ThumbnailImage;

	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	bool      bFlying     = false;
	float     FlyTimer    = 0.f;
	float     FlyDuration = 0.45f;
	FVector2D FlyStartAbs;
	FVector2D FlyEndAbs;
	FVector2D ImgSize = FVector2D(64.f, 64.f);

	UPROPERTY()
	TObjectPtr<UTexture2D> CachedThumbnail;
};
