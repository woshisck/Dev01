#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Data/RuneDataAsset.h"
#include "RuneInfoCardWidget.generated.h"

class UImage;
class UTextBlock;
class UCanvasPanel;
class UCanvasPanelSlot;

/**
 * 符文信息卡 Widget
 *
 * 蓝图制作步骤：
 *   1. 新建 Widget Blueprint，父类选 RuneInfoCardWidget
 *   2. Designer 里放以下控件（名称必须完全一致）：
 *        CardIcon    Image     符文图标
 *        CardName    TextBlock 符文名
 *        CardDesc    TextBlock 符文描述
 *        CardUpgrade TextBlock 升级等级（满级时自动隐藏）
 *   3. 在 WBP_BackpackScreen 的 Canvas Panel 里放本 Widget 实例，
 *      命名为 RuneInfoCard，Visibility 设为 Collapsed
 */
UCLASS(Blueprintable, BlueprintType)
class DEVKIT_API URuneInfoCardWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    // =========================================================
    // Designer 绑定（名称必须完全一致）
    // =========================================================

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UImage> CardBG;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UImage> CardIcon;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> CardName;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> CardDesc;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> CardUpgrade;

    /**
     * 符文占格点阵（UniformGridPanel，命名 "ShapeGrid"）。
     * C++ 根据 FRuneShape 动态生成点阵子节点，无需在 Designer 里手工填充内容。
     * 建议在 Designer 里给它设一个固定大小（如 120×120）。
     */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UCanvasPanel> ShapeGrid;

    /** 效果描述（命名 "CardEffect"），与 CardDesc 区分显示 */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> CardEffect;

    // =========================================================
    // 对外接口（BackpackScreenWidget 调用）
    // =========================================================

    /** 显示指定符文信息 */
    UFUNCTION(BlueprintCallable, Category = "RuneInfoCard")
    void ShowRune(const FRuneInstance& Rune);

    /** 隐藏卡片 */
    UFUNCTION(BlueprintCallable, Category = "RuneInfoCard")
    void HideCard();

private:
    /** 根据 FRuneShape 重建 ShapeGrid 子节点（点阵） */
    void BuildShapeGrid(const FRuneShape& Shape);
};
