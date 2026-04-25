#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "YogHUDRootWidget.generated.h"

class ULiquidHealthBarWidget;
class UEnemyArrowWidget;
class UWeaponGlassIconWidget;
class UHeatBarWidget;
class UInfoPopupWidget;

/**
 * 主 HUD 容器 Widget（C++ 基类）
 *
 * 对应 WBP_HUDRoot：内含 Canvas Panel + 所有常驻 HUD 子控件。
 * 子控件名称必须与 BindWidget 字段名完全一致。
 *
 * 新增 HUD 元素时：
 *   1. 在 WBP_HUDRoot Canvas Panel 里拖入目标控件，重命名（Is Variable 勾选）
 *   2. 在此处加一行 BindWidgetOptional UPROPERTY
 *   3. 在 YogHUD.cpp 里通过 MainHUDWidget->NewWidget 访问
 */
UCLASS()
class DEVKIT_API UYogHUDRootWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    /** 玩家液态血条（WBP 控件变量名：PlayerHealthBar） */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    TObjectPtr<ULiquidHealthBarWidget> PlayerHealthBar;

    /** 敌人方向指示箭头 - 全屏覆盖（WBP 控件变量名：EnemyArrow） */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UEnemyArrowWidget> EnemyArrow;

    /** 武器玻璃图标（WBP 控件变量名：WeaponGlassIcon） */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UWeaponGlassIconWidget> WeaponGlassIcon;

    /** 热度条（WBP 控件变量名：HeatBar） */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UHeatBarWidget> HeatBar;

    /** 轻量信息提示浮窗（WBP 控件变量名：InfoPopup） */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UInfoPopupWidget> InfoPopup;
};
