#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Data/RuneDataAsset.h"
#include "WeaponFloatWidget.generated.h"

DECLARE_DELEGATE_OneParam(FOnWeaponFloatCollapseComplete, FVector2D /*ThumbnailScreenCenter*/)

class UImage;
class UTextBlock;
class UCanvasPanel;
class UVerticalBox;
class UWidget;
class UCommonRichTextBlock;
class UYogCommonRichTextBlock;
class UWeaponDefinition;
struct FRuneShape;

/**
 * 武器拾取浮窗 Widget — 纯信息展示，飞行动画由 WeaponThumbnailFlyWidget 负责。
 *
 * WBP 需命名以下控件（全部 BindWidgetOptional）：
 *   WeaponThumbnail   Image                  武器缩略图
 *   InfoContainer     Widget                 包裹名称/描述/符文的容器
 *   WeaponNameText    TextBlock              武器名称
 *   WeaponDescText    YogCommonRichTextBlock 武器描述（空时自动隐藏；支持 <input action="X"/> 图标）
 *   WeaponSubDescText YogCommonRichTextBlock 武器子描述（同上）
 *   ZoneGrid1/2/3     CanvasPanel            激活区点阵（建议 60×60）
 *   Zone1Image/2/3    Image                  激活区图像覆盖（提供时替代点阵）
 *   RuneListBox       VerticalBox            初始符文列表（C++ 动态填充）
 *   PickupHintText    YogCommonRichTextBlock 按键拾取提示（如 `按 <input action="Interact"/> 拾取武器`）
 */
UCLASS(Blueprintable, BlueprintType)
class DEVKIT_API UWeaponFloatWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "WeaponFloat")
	void SetWeaponDefinition(const UWeaponDefinition* Def);

	/** 返回缩略图贴图（供外部获取飞行起点使用） */
	UFUNCTION(BlueprintPure, Category = "WeaponFloat")
	UTexture2D* GetCachedThumbnailTexture() const { return CachedThumbnail; }

	/** 折叠动画：InfoContainer 淡出，完成后广播缩略图屏幕中心坐标 */
	void StartCollapse(float InDuration = 0.25f);

	/** 折叠完成时触发（传入 WeaponThumbnail 的屏幕绝对中心坐标） */
	FOnWeaponFloatCollapseComplete OnCollapseComplete;

protected:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UImage> WeaponThumbnail;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UWidget> InfoContainer;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> WeaponNameText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UYogCommonRichTextBlock> WeaponDescText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UYogCommonRichTextBlock> WeaponSubDescText;

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

	/** 拾取按键提示文字（支持 <input action="Interact"/> 显示按键图标） */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UCommonRichTextBlock> PickupHintText;

private:
	void BuildZonePanel(UCanvasPanel* GridPanel, UImage* ImgWidget,
	                    UTexture2D* ZoneTexture, const FRuneShape* Shape,
	                    int32 GW, int32 GH);

	void BuildRuneList(const TArray<TObjectPtr<URuneDataAsset>>& Runes);

	UPROPERTY()
	TObjectPtr<UTexture2D> CachedThumbnail;

	bool  bCollapsing      = false;
	float CollapseTimer    = 0.f;
	float CollapseDuration = 0.25f;
};
