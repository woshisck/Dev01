#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Data/RuneDataAsset.h"
#include "WeaponFloatWidget.generated.h"

class UImage;
class UTextBlock;
class UCanvasPanel;
class UVerticalBox;
class UWidget;
class UWeaponDefinition;
class UWeaponGlassAnimDA;
struct FRuneShape;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeaponFlyComplete, UTexture2D*, CachedThumbnail);

/** 飞行阶段每帧广播 (飞行起点, 当前绝对坐标, 进度 0-1)，供拖尾 Widget 使用 */
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnWeaponFlyProgress, FVector2D, FVector2D, float);

/** 浮窗动画阶段 */
UENUM()
enum class EWeaponFloatPhase : uint8
{
	Idle,       // 正常展示
	Collapsing, // 隐藏文字/符文区域
	Shrinking,  // 缩放至玻璃图标尺寸
	Flying,     // 平移飞向 HUD 锚点
};

/**
 * 武器拾取浮窗 Widget
 *
 * WBP 需命名以下控件（全部 BindWidgetOptional）：
 *   WeaponThumbnail  Image        武器缩略图（折叠/飞行时保留）
 *   InfoContainer    Widget       包裹名称/描述/符文的容器（折叠时整体隐藏）
 *   WeaponNameText   TextBlock    武器名称
 *   WeaponDescText   TextBlock    武器描述（空时自动隐藏）
 *   WeaponSubDescText TextBlock   武器子描述
 *   ZoneGrid1/2/3    CanvasPanel  激活区点阵（建议 60×60）
 *   Zone1Image/2/3   Image        激活区图像覆盖（提供时替代点阵）
 *   RuneListBox      VerticalBox  初始符文列表（C++ 动态填充）
 *
 * 动画流程（C++ Tick 驱动，参数由 WeaponGlassAnimDA 控制）：
 *   StartCollapseAndFly → Collapsing → Shrinking → Flying → OnFlyComplete 广播
 */
UCLASS(Blueprintable, BlueprintType)
class DEVKIT_API UWeaponFloatWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "WeaponFloat")
	void SetWeaponDefinition(const UWeaponDefinition* Def);

	/**
	 * 开始折叠→缩小→飞行动画序列
	 * @param TargetScreenCenter  飞行终点屏幕坐标（中心点）
	 * @param InAnimDA            动画参数 DA
	 */
	UFUNCTION(BlueprintCallable, Category = "WeaponFloat")
	void StartCollapseAndFly(FVector2D TargetScreenCenter, const UWeaponGlassAnimDA* InAnimDA);

	/** 飞行结束后广播（参数为缓存的缩略图贴图，可直接传给 WeaponGlassIconWidget） */
	UPROPERTY(BlueprintAssignable, Category = "WeaponFloat")
	FOnWeaponFlyComplete OnFlyComplete;

	/** 飞行中每帧广播 (FlyAbsStart, CurrentAbsPos, Alpha)，供 WeaponTrailWidget 更新线段 */
	FOnWeaponFlyProgress OnFlyProgress;

	/** 当前动画阶段（只读，供 BP 查询） */
	UFUNCTION(BlueprintPure, Category = "WeaponFloat")
	EWeaponFloatPhase GetFloatPhase() const { return CurrentPhase; }

protected:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	// ──────────────────────────────────────────
	//  BindWidgetOptional（WBP 按名称绑定）
	// ──────────────────────────────────────────

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UImage> WeaponThumbnail;

	/**
	 * 包裹所有非缩略图内容的容器（折叠阶段整体隐藏）
	 * WBP 里将 WeaponNameText / ZoneGrid / RuneListBox 等放入此容器并命名 "InfoContainer"
	 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UWidget> InfoContainer;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> WeaponNameText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> WeaponDescText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> WeaponSubDescText;

	// 点阵填充尺寸（px）：与 WBP 里 ZoneGrid SizeBox 的宽高保持一致
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WeaponFloat", meta = (ClampMin = "20"))
	float ZoneGridSize = 60.f;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UCanvasPanel> ZoneGrid1;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UCanvasPanel> ZoneGrid2;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UCanvasPanel> ZoneGrid3;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UImage> Zone1Image;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UImage> Zone2Image;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UImage> Zone3Image;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> RuneListBox;

private:
	void BuildZonePanel(UCanvasPanel* GridPanel, UImage* ImgWidget,
	                    UTexture2D* ZoneTexture, const FRuneShape* Shape,
	                    int32 GW, int32 GH);

	void BuildRuneList(const TArray<TObjectPtr<URuneDataAsset>>& Runes);

	// ── 动画状态 ──────────────────────────────
	EWeaponFloatPhase CurrentPhase = EWeaponFloatPhase::Idle;

	UPROPERTY()
	TObjectPtr<const UWeaponGlassAnimDA> AnimDA;

	float PhaseTimer        = 0.f;
	float TargetShrinkScale = 1.f;
	FVector2D FlyDelta;              // 飞行位移（屏幕像素）

	// 飞行起点（绝对屏幕坐标），Flying 首帧捕获
	FVector2D FlyAbsStart      = FVector2D::ZeroVector;
	bool      bFlyStartCaptured = false;

	// 缓存缩略图，飞行完成后传给 WeaponGlassIconWidget
	UPROPERTY()
	TObjectPtr<UTexture2D> CachedThumbnail;
};
