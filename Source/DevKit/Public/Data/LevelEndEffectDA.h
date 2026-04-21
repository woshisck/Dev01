#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "LevelEndEffectDA.generated.h"

/**
 * 关卡结束视觉特效参数 DA。
 * 在 BP_YogHUD 的 LevelEndEffectDA 槽位指定实例。
 *
 * 时序（真实时间）：
 *   [0, BlackoutFadeDuration)  — 画面渐黑
 *   [BlackoutFadeDuration, SlowMoDuration) — 保持全黑
 *   t == SlowMoDuration        — 恢复正常时速，开始圆形揭幕
 *   [SlowMoDuration, SlowMoDuration + RevealDuration) — 圆形扩散
 */
UCLASS(BlueprintType)
class DEVKIT_API ULevelEndEffectDA : public UDataAsset
{
	GENERATED_BODY()

public:
	// ── 时间膨胀 ─────────────────────────────────────────────────────────

	// 时间膨胀缩放（0.2 = 五分之一速）
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TimeDilation", meta = (ClampMin = "0.01", ClampMax = "1.0"))
	float SlowMoScale = 0.2f;

	// 慢动作持续时长（真实时间，秒）
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TimeDilation", meta = (ClampMin = "0.1"))
	float SlowMoDuration = 0.5f;

	// ── 变黑效果 ─────────────────────────────────────────────────────────

	// 画面变黑渐入时长（真实时间，应 < SlowMoDuration）
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Blackout", meta = (ClampMin = "0.05"))
	float BlackoutFadeDuration = 0.2f;

	// 目标饱和度（0 = 完全灰度，1 = 不变）
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Blackout", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float BlackoutSaturation = 0.0f;

	// 目标亮度增益（0 = 全黑，1 = 不变）
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Blackout", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float BlackoutGain = 0.0f;

	// ── 圆形揭幕 ─────────────────────────────────────────────────────────

	// 揭幕持续时长（真实时间，秒），从时间恢复后开始
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CircleReveal", meta = (ClampMin = "0.1"))
	float RevealDuration = 0.4f;

	// 圆形边缘软硬（数值越大边缘越硬，推荐 20~80）
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CircleReveal", meta = (ClampMin = "1.0"))
	float RevealEdgeSharpness = 40.0f;

	/**
	 * 圆形揭幕材质（UI Domain）
	 *
	 * 材质需要以下参数：
	 *   RevealCenter  (Vector)  — 揭幕中心 UV（0~1 范围，XY）
	 *   RevealProgress (Scalar) — 0=全黑，1=全透明
	 *   EdgeSharpness  (Scalar) — 边缘硬度
	 *
	 * 材质逻辑（Custom Node 或节点网络）：
	 *   UV = TexCoord[0]
	 *   Dist = length(UV - RevealCenter.xy)
	 *   MaxDist = 1.5  // 足够覆盖所有屏幕角落
	 *   Alpha = saturate((Dist - RevealProgress * MaxDist) * EdgeSharpness)
	 *   → Opacity Mask = Alpha（遮住黑色底色）
	 *   → Final Color = (0,0,0) Emissive
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CircleReveal")
	TObjectPtr<UMaterialInterface> RevealMaterial;
};
