#pragma once

#include "CoreMinimal.h"
#include "YogAirFogVolume.h"
#include "YogAirFogWorkbenchDraft.generated.h"

/** Local editor draft. It never changes a map, asset or project default. */
UCLASS(Transient)
class YOGVISIONEDITOR_API UYogAirFogWorkbenchDraft : public UObject
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, Category="预览组合", meta=(DisplayName="启用远景结构修饰", ToolTip="将远景压低对比与弱结构描边叠加在空气雾上。关闭后可单独观察空气体积；此开关随命名草稿保存。"))
    bool bFinishEnabled = true;

    UPROPERTY(EditAnywhere, Category="美术配色", meta=(DisplayName="配色材质", ToolTip="使用美术材质配色时读取该持久材质的近远雾色与lerp_fog。材质资产身份随命名草稿保存；不能保存临时MID。"))
    TSoftObjectPtr<UMaterialInterface> PaletteMaterial;

    /** Transactional document identity; disk-save baselines live in the widget. */
    UPROPERTY(Transient)
    FString PresetId;

    UPROPERTY(EditAnywhere, Category="美术草稿", meta=(ShowOnlyInnerProperties, DisplayName="空气雾参数", ToolTip="空气雾、近处薄空气和镜头近区残留均为每米的绝对密度，满足残留不高于近处、近处不高于空气雾；空气雾密度为0时全部归零。"))
    FYogAirFogParameters Parameters;
};
