#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "WeaponTrailWidget.generated.h"

class UImage;

/**
 * 全屏透明叠层，在武器拾取飞行阶段绘制一条发光流光线段。
 * WBP 根节点为全屏 CanvasPanel（HitTestInvisible），内含名为 "TrailLine" 的 Image。
 * 飞行结束后调用 StartFadeOut()，自动淡出并从视口移除。
 */
UCLASS()
class DEVKIT_API UWeaponTrailWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 每帧由 YogHUD 调用，更新线段起终点和材质进度 */
	void SetTrailEndpoints(FVector2D Start, FVector2D Current, float Alpha);

	/** 飞行完成后触发淡出 */
	void StartFadeOut();

	// ── WBP 绑定 ──────────────────────────────
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> TrailLine;

	// ── 编辑器配置 ────────────────────────────
	/** 发光线段材质（UI Domain，需暴露 Alpha / Progress 标量参数） */
	UPROPERTY(EditDefaultsOnly, Category = "Trail")
	TObjectPtr<UMaterialInterface> TrailMaterial;

	/** 线段粗细（像素） */
	UPROPERTY(EditDefaultsOnly, Category = "Trail", meta = (ClampMin = "1"))
	float LineHeight = 8.f;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> TrailDynMat;

	bool  bFadingOut  = false;
	float FadeTimer   = 0.f;

	static constexpr float FadeDuration = 0.25f;
};
