#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Map/Portal.h"   // FPortalPreviewInfo
#include "PortalPreviewWidget.generated.h"

class UTextBlock;
class UBorder;
class UVerticalBox;
class UWidget;

/**
 * 传送门下一关预览浮窗（HUD 单例）。
 *
 * 由 AYogHUD 管理生命周期，按"距玩家最近的开启 Portal"为目标显示；
 * 玩家进入 Box 时追加"按 E 进入"提示。
 *
 * WBP 控件（全部 BindWidgetOptional）：
 *   RoomNameText      TextBlock     房间显示名（DisplayName 或 RoomName 兜底）
 *   RoomTypeBadge     Border        类型徽章背景色（C++ 按 Tag 写入）
 *   RoomTypeText      TextBlock     类型文字（"普通/精英/商店/事件"）
 *   BuffListBox       VerticalBox   动态填充每个预骰 Buff 行（图标 + 名称）
 *   LootSummaryText   TextBlock     "战利品：符文 ×3"
 *   InteractHintRoot  Widget        "按 E 进入"容器（SetInteractHintVisible 控制）
 */
UCLASS(Blueprintable, BlueprintType)
class DEVKIT_API UPortalPreviewWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    /** 由 HUD 在 Target Portal 切换时调用，刷新整张浮窗内容 */
    UFUNCTION(BlueprintCallable, Category = "PortalPreview")
    void SetPreviewInfo(const FPortalPreviewInfo& Info);

    /** HUD 检测到 PendingPortal == 当前 Target 时调 true，否则 false */
    UFUNCTION(BlueprintCallable, Category = "PortalPreview")
    void SetInteractHintVisible(bool bVisible);

    // === BP 视觉强化钩 ===

    UFUNCTION(BlueprintImplementableEvent, Category = "PortalPreview|FX",
              meta = (DisplayName = "On Preview Shown"))
    void K2_OnPreviewShown();

    UFUNCTION(BlueprintImplementableEvent, Category = "PortalPreview|FX",
              meta = (DisplayName = "On Preview Hidden"))
    void K2_OnPreviewHidden();

    UFUNCTION(BlueprintImplementableEvent, Category = "PortalPreview|FX",
              meta = (DisplayName = "On Interact Hint Shown"))
    void K2_OnInteractHintShown();

protected:
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> RoomNameText;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UBorder> RoomTypeBadge;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> RoomTypeText;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UVerticalBox> BuffListBox;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> LootSummaryText;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UWidget> InteractHintRoot;

private:
    bool bInteractHintVisible = false;
};
