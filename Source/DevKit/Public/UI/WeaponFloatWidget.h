#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Data/RuneDataAsset.h"
#include "WeaponFloatWidget.generated.h"

class UImage;
class UTextBlock;
class UCanvasPanel;
class UVerticalBox;
class UWeaponDefinition;
struct FRuneShape;

/**
 * 武器拾取浮窗 Widget
 *
 * WBP 需命名以下控件（全部 BindWidgetOptional）：
 *   WeaponThumbnail  Image        武器缩略图
 *   WeaponNameText   TextBlock    武器名称
 *   WeaponDescText   TextBlock    武器描述（空时自动隐藏）
 *   ZoneGrid1/2/3    CanvasPanel  激活区点阵（建议 60×60）
 *   Zone1Image/2/3   Image        激活区图像覆盖（提供时替代点阵）
 *   RuneListBox      VerticalBox  初始符文列表（C++ 动态填充）
 */
UCLASS(Blueprintable, BlueprintType)
class DEVKIT_API UWeaponFloatWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "WeaponFloat")
	void SetWeaponDefinition(const UWeaponDefinition* Def);

protected:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UImage> WeaponThumbnail;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> WeaponNameText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> WeaponDescText;

	// 三个激活区点阵容器（ZoneGridN 与 ZoneNImage 互斥：有图像时隐藏点阵）
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

	// 初始符文列表容器（C++ 动态生成 HorizontalBox 子项）
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> RuneListBox;

private:
	void BuildZonePanel(UCanvasPanel* GridPanel, UImage* ImgWidget,
	                    UTexture2D* ZoneTexture, const FRuneShape* Shape,
	                    int32 GW, int32 GH);

	void BuildRuneList(const TArray<TObjectPtr<URuneDataAsset>>& Runes);
};
