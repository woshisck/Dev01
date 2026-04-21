#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LevelEndRevealWidget.generated.h"

class UImage;
class UMaterialInterface;
class UMaterialInstanceDynamic;

/**
 * 关卡结束圆形揭幕遮罩 Widget。
 * WBP 中放一个 Image 命名 "RevealImage"，HAlign/VAlign=Fill，使用 RevealMaterial。
 * 材质参数由 YogHUD::Tick 每帧驱动，此类只负责初始化。
 */
UCLASS()
class DEVKIT_API ULevelEndRevealWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// WBP 中 Image 命名必须为 "RevealImage"，全屏 Fill
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> RevealImage;

	/**
	 * 初始化材质实例。由 YogHUD 在 StartReveal 时调用。
	 * @return 创建好的 DynMat，YogHUD 每帧更新 RevealProgress 参数
	 */
	UMaterialInstanceDynamic* InitReveal(UMaterialInterface* Mat,
	                                     FVector2D LootScreenUV,
	                                     float EdgeSharpness);
};
